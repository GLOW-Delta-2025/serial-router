#include <Arduino.h>
#include "../lib/CmdLib.h"  // <= uses your uploaded command library

using namespace cmdlib;

// ---------- Forward declarations ----------
void routeFromMac(const String &raw);
void routeFromPort(HardwareSerial &port, int sourceId);

// ---------- Define serial ports ----------
#define mac Serial
#define arm1 Serial6
#define arm2 Serial2
#define arm3 Serial3
#define arm4 Serial4
#define arm5 Serial5
#define Centerpiece Serial1

// Numeric IDs: 0=MASTER (Mac), 1..5=ARM1..ARM5, 6=CENTERPIECE
static inline String sourceLabel(int id) {
  if (id == 0) return "MASTER";
  if (id >= 1 && id <= 5) return String("ARM") + String(id);
  if (id == 6) return "CENTERPIECE";
  return "UNKNOWN";
}

// ---------- Setup ----------
void SetupSerialRouting() {
  mac.begin(9600);
  arm1.begin(9600);
  arm2.begin(9600);
  arm3.begin(9600);
  arm4.begin(9600);
  arm5.begin(9600);
  Centerpiece.begin(9600);
}

// ---------- Send Helpers ----------
static inline void sendToArm(int armNumber, const String &message) {
  switch (armNumber) {
    case 1: arm1.print(message); break;
    case 2: arm2.print(message); break;
    case 3: arm3.print(message); break;
    case 4: arm4.print(message); break;
    case 5: arm5.print(message); break;
    default: break;
  }
}


static inline void sendToCenterpiece(const String &message) { Centerpiece.print(message); }
static inline void sendToMac(const String &message) { mac.print(message); }

// ---------- CmdLib helpers ----------
static bool headersHas(const Command &cmd, const String &needle) {
  for (int i = 0; i < cmd.headerCount; ++i)
    if (cmd.headers[i] == needle) return true;
  return false;
}

static int headersFindArm(const Command &cmd) {
  for (int i = 0; i < cmd.headerCount; ++i) {
    const String h = cmd.headers[i];
    if      (h == "ARM1") return 1;
    else if (h == "ARM2") return 2;
    else if (h == "ARM3") return 3;
    else if (h == "ARM4") return 4;
    else if (h == "ARM5") return 5;
  }
  return 0;
}

static String pickDestinationHeader(const Command &cmd, const String &exclude) {
  // Prefer explicit known destinations; ignore the source "exclude"
  for (int i = 0; i < cmd.headerCount; ++i) {
    const String h = cmd.headers[i];
    if (h == exclude) continue;
    if (h == "MASTER" || h == "CENTERPIECE" ||
        h == "ARM1" || h == "ARM2" || h == "ARM3" || h == "ARM4" || h == "ARM5")
      return h;
  }
  // If none matched, but there were headers, pick the first non-excluded
  for (int i = 0; i < cmd.headerCount; ++i) {
    if (cmd.headers[i] != exclude) return cmd.headers[i];
  }
  return ""; // no destination present
}

static void copyPayload(const Command &src, Command &dst) {
  dst.msgKind = src.msgKind;
  dst.command = src.command;
  for (int i = 0; i < src.namedCount; ++i)
    dst.setNamed(src.namedParams[i].key, src.namedParams[i].value);
}

// ---------- Build a strict FROM:TO command ----------
static bool buildFromTo(const Command &in, int sourceId, Command &out, String &toHeader) {
  const String fromHeader = sourceLabel(sourceId);
  toHeader = pickDestinationHeader(in, fromHeader);
  if (toHeader.length() == 0) return false; // can't route

  out.clear();
  out.addHeader(fromHeader);
  out.addHeader(toHeader);
  copyPayload(in, out);
  return true;
}

// ---------- Logging ----------
static void logParsed(const char *tag, const Command &cmd) {
  mac.print(tag);
  mac.print(" headers=");
  for (int i = 0; i < cmd.headerCount; ++i) {
    if (i) mac.print(",");
    mac.print(cmd.headers[i]);
  }
  mac.print(" kind="); mac.print(cmd.msgKind);
  mac.print(" cmd=");  mac.print(cmd.command);
  if (cmd.namedCount > 0) {
    mac.print(" params={");
    for (int i = 0; i < cmd.namedCount; ++i) {
      if (i) mac.print(",");
      mac.print(cmd.namedParams[i].key); mac.print("=");
      mac.print(cmd.namedParams[i].value);
    }
    mac.print("}");
  }
  mac.println();
}


// ---------- Routing core ----------
static void deliverByDestination(const String &toHeader, const String &framed) {
  if (toHeader == "MASTER") {
    mac.print("[→ MASTER] "); mac.println(framed);
    sendToMac(framed);
  } else if (toHeader == "CENTERPIECE") {
    sendToCenterpiece(framed);
    mac.print("[→ CENTERPIECE] "); mac.println(framed);
  } else if (toHeader.startsWith("ARM")) {
    int arm = 0;
    if (toHeader == "ARM1") arm = 1;
    else if (toHeader == "ARM2") arm = 2;
    else if (toHeader == "ARM3") arm = 3;
    else if (toHeader == "ARM4") arm = 4;
    else if (toHeader == "ARM5") arm = 5;
    if (arm >= 1 && arm <= 5) {
      sendToArm(arm, framed);
      mac.print("[→ ARM"); mac.print(arm); mac.print("] "); mac.println(framed);
      return;
    }
    mac.print("[ERROR] Invalid arm destination"); mac.println(framed);
  } else {
    mac.print("[ERROR] Invalid destination "); mac.println(framed);
  }
}

// ---------- Routing from Mac (sourceId=0 -> MASTER) ----------
void routeFromMac(const String &raw) {
  Command in;
  String err;
  if (!parse(raw, in, err)) {
    mac.print("[ERROR] Parse failed: "); mac.println(err);
    return;
  }

  logParsed("[OK]", in);

  Command out;
  String toHeader;
  if (!buildFromTo(in, /*sourceId=*/0, out, toHeader)) {
    mac.print("[ERROR] No destination "); mac.println(in.toString());
    return;
  }

  const String framed = out.toString();
  deliverByDestination(toHeader, framed);
}

// ---------- Routing from any actor port (ARMs=1..5, CENTERPIECE=6) ----------
void routeFromPort(HardwareSerial &port, int sourceId) {
  static String buffers[7];  // 0=MASTER (unused), 1..5=arms, 6=centerpiece
  String &buffer = buffers[sourceId];

  while (port.available()) {
    char ch = (char)port.read();
    buffer += ch;

    if (buffer.endsWith("##")) {
      int startIdx = buffer.lastIndexOf("!!");
      String candidate = (startIdx >= 0) ? buffer.substring(startIdx) : buffer;

      Command in;
      String err;
      if (!parse(candidate, in, err)) {
        mac.print("[✖ PARSE from "); mac.print(sourceLabel(sourceId)); mac.print("] ");
        mac.println(err);
      } else {
        // Always rebuild to strict FROM:TO with FROM = source port label
        Command out;
        String toHeader;
        if (!buildFromTo(in, sourceId, out, toHeader)) {
          mac.print("[⚠ NO DEST from "); mac.print(sourceLabel(sourceId)); mac.print("] ");
          mac.println(in.toString());
        } else {
          const String framed = out.toString();

          // Log what we received and what we'll deliver
          mac.print("[← "); mac.print(sourceLabel(sourceId)); mac.print("] ");
          mac.println(framed);

          // Forward to destination
          if (out.msgKind == "REQUEST") {}
           deliverByDestination(toHeader, framed);
        }
      }

      buffer = "";
      port.flush();
    }
  }
}

// ---------- Main ----------
void setup() {
  SetupSerialRouting();
  while (!mac) { ; } // wait for USB-serial

  mac.println("Serial router ready (FROM:TO enforced).");
  mac.println("Examples to type from Mac:");
  mac.println("  !!ARM1:REQUEST:MAKE_STAR{size=120,color=RED,100,6}##   -> !!MASTER:ARM1:...##");
  mac.println("  !!CENTERPIECE:REQUEST:PING{}##                         -> !!MASTER:CENTERPIECE:...##");
}

void loop() {
  static String macBuffer;

  // Read from Mac (USB)
  while (mac.available()) {
    char ch = (char)mac.read();
    macBuffer += ch;
    if (macBuffer.endsWith("##")) {
      int startIdx = macBuffer.lastIndexOf("!!");
      String candidate = (startIdx >= 0) ? macBuffer.substring(startIdx) : macBuffer;
      routeFromMac(candidate);
      macBuffer = "";
    }
  }

  // Read from actors (FROM is implied by port)
  routeFromPort(arm1, 1);
  routeFromPort(arm2, 2);
  routeFromPort(arm3, 3);
  routeFromPort(arm4, 4);
  routeFromPort(arm5, 5);
  routeFromPort(Centerpiece, 6);
}
