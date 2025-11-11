# Echoes of Tomorrow – Teensy Serial Router Firmware

Acceptance Test Plan and Integration Plan – Technical Report

Document ID: EOT-TSR-ATP-IP
Version: 0.2 (Draft)
Date: 2025-10-22
Authors: Firmware & Systems Team
Reviewers: Production Engineering, Interactive Media, QA

## Executive Summary
This technical report defines how the Teensy 4.1–based Serial Router firmware will be verified for acceptance and integrated into the Echoes of Tomorrow system-of-systems. The router mediates bidirectional command traffic between the Mac Mini (show controller) and distributed ESP-based subsystems (arms) plus a centerpiece controller. The report consolidates objectives, protocol constraints, verification methodology, acceptance criteria, and a staged, abstract integration plan. It references the project’s Serial Communication Agreement to ensure protocol compliance and operational safety.

## 1. Introduction and Context
The Serial Router is the communication backbone for interactive choreography across multiple microcontroller nodes. It performs: (a) parsing of framed commands; (b) routing to addressed endpoints; (c) optional automatic acknowledgement; and (d) telemetry and diagnostics over USB. Acceptance focuses on functional correctness, robustness under malformed traffic, and timing behavior at specified baud rates. Integration proceeds incrementally from bench validation to rehearsal-grade system tests.

### 1.1 Scope
- In scope: Firmware build `teensy4-1_routing`; parsing and routing (CmdLib); UART links to arms and centerpiece; USB logging/diagnostics; watchdog/health behavior.
- Out of scope: Mac Mini application logic; ESP firmware internals; mechanical validation; cloud/backends.

### 1.2 References
- GLOW Hardware Communication Agreements (Serial Communication Agreement), latest approved revision.
- Repo: `serial-router`, branch `teensy4-1_routing`.
- Source: `serial_routing/src/main.cpp`, `serial_routing/lib/CmdLib.h`.

### 1.3 Assumptions & Constraints
- Message framing: start token `!!`, field delimiter `:`, terminator `##`.
- Command verbs: e.g., `REQUEST`, `CONFIRM` per current agreement; additional verbs TBD.
- Baud rates: USB CDC (Mac link) at 115200; UART links default 9600 unless otherwise specified by device.
- Electrical: UART TTL levels compatible with Teensy 4.1; optional GPIO handshakes may be introduced (TBD).

## 2. System Under Test (SUT)
### 2.1 Role and Responsibilities
The Teensy 4.1 receives framed commands, validates structure, determines destination, forwards over the appropriate UART, and relays responses to the Mac Mini. Where specified, it transforms `REQUEST` to `CONFIRM` for auto-acknowledge flows.

### 2.2 Architecture Overview
- Upstream: Mac Mini via USB CDC serial (`Serial`).
- Downstream UART links: five ESP endpoints and a centerpiece controller.
- Routing logic encapsulated via CmdLib (named-parameter variant) and firmware in `main.cpp`.

### 2.3 Interface Inventory (Teensy 4.1)
- USB: `Serial` (Mac link)
- UART: `Serial1` (Centerpiece), `Serial2` (Arm2), `Serial3` (Arm3), `Serial4` (Arm4), `Serial5` (Arm5), `Serial6` (Arm1)

| Logical Port | Teensy Serial | Default Baud | Pins (T4.1) |
|---|---|---|---|
| Mac | `Serial` | 115200 | USB microcontroller port |
| Centerpiece | `Serial1` | 9600 | RX1=0, TX1=1 |
| Arm1 | `Serial6` | 9600 | RX6=25, TX6=24 |
| Arm2 | `Serial2` | 9600 | RX2=7, TX2=8 |
| Arm3 | `Serial3` | 9600 | RX3=15, TX3=14 |
| Arm4 | `Serial4` | 9600 | RX4=16, TX4=17 |
| Arm5 | `Serial5` | 9600 | RX5=21, TX5=20 |

Note: GPIO handshakes/interrupt lines are currently not mandated by the protocol; placeholders exist to add them if needed.

## 3. Communication Protocol Summary (based on Agreement)
The Serial Communication Agreement is the authoritative source for protocol rules; this report verifies conformance to that document without restating it. It defines message framing and delimiters, addressing/routing headers, verb semantics (e.g., REQUEST/CONFIRM), payload constraints and escaping, error codes, and timing/baud specifications used to derive the acceptance criteria in §5.

## 4. Verification Approach
Verification follows a layered approach: static checks, bench functional tests, robustness and fault-injection, and performance characterization. Evidence is gathered via USB logs, UART taps, and (optionally) a logic analyzer.

### 4.1 Test Environment and Instrumentation
- Hardware: Teensy 4.1, Mac Mini, up to five ESP32 modules (or UART emulators), centerpiece controller/emulator.
- Software: PlatformIO/Teensy Loader; Python harness (`Serial_Router.py`); `screen`/CoolTerm; optional logic analyzer.
- Configuration control: Git-referenced firmware commit; documented wiring map; baud rates recorded per run.

### 4.2 Entry/Exit Criteria
- Entry: Build compiles; device flashes; bench wiring verified; sanity smoke passes (power-on self-log).
- Exit: All Acceptance Criteria (§5) met; no open Sev1/Sev2 defects; Sev3 reviewed and accepted.

## 5. Acceptance Criteria
1. Protocol Conformance: All accepted frames adhere to `!!...##`, field structure, and routing header semantics.
2. Routing Correctness: Each destination receives frames losslessly and unaltered (except deliberate verb transforms).
3. Robustness: Malformed frames and noise do not crash or deadlock the router; subsequent valid traffic proceeds normally.
4. Performance: End-to-end latency remains within calculated line time + 20 ms router overhead (unless otherwise specified by subsystem needs).
5. Observability: USB logs provide timestamped traces sufficient for incident reconstruction without overwhelming the link.
6. Safety: No unintended cross-port broadcast; messages are delivered exactly once to the addressed endpoint.

## 6. Acceptance Test Design and Procedures
Each procedure specifies purpose, setup, method, and acceptance checks. Representative procedures are listed; the complete catalog is maintained in the QA tracker and can be expanded.

### AT-001: Power-On and Readiness Announcement
- Purpose: Verify boot diagnostics and readiness banner.
- Setup: Teensy flashed; USB connected; serial monitor open at 115200.
- Method: Power-cycle; observe first 5 seconds of output.
- Acceptance: Banner present; no stack traces; ports initialized (baud settings logged).

### AT-002: Mac→Arm Unicast Routing
- Purpose: Confirm unicast delivery to Arm1.
- Setup: ESP/emulator on Arm1 (Serial6, 9600); Mac console open.
- Method: Send `!![ARM1]:REQUEST:STATUS##` from Mac; observe Arm1 UART.
- Acceptance: Frame appears on Arm1 with identical payload; Mac receives any Arm1 response without corruption.

### AT-003: Arm→Mac Return Path
- Purpose: Validate upstream delivery without mutation.
- Setup: ESP on Arm2.
- Method: ESP sends `!!MASTER:REQUEST:MAKE_STAR##`.
- Acceptance: Mac receives same frame; no duplicates; timestamp delta consistent with line time.

### AT-004: Auto-Acknowledge Transform
- Purpose: Verify `REQUEST→CONFIRM` behavior when configured.
- Setup: ESP on Arm3.
- Method: Send `!!MASTER:REQUEST:MAKE_STAR##`.
- Acceptance: ESP receives `!!MASTER:CONFIRM:MAKE_STAR##`; Mac log records action.

### AT-005: Centerpiece Addressing
- Purpose: Validate directed delivery to centerpiece only.
- Setup: Centerpiece on Serial1.
- Method: Send `!!CENTERPIECE:REQUEST:SYNC##` from Mac.
- Acceptance: Appears on Serial1 only; no frames on other arm ports.

### AT-006: Unknown Destination Handling
- Purpose: Confirm graceful rejection of unknown routes.
- Method: Send `!![UNKNOWN]:REQUEST:TEST##` from Mac.
- Acceptance: Router emits error/diagnostic upstream; no crash; router continues processing subsequent frames.

### AT-007: Throughput and Backpressure
- Purpose: Assess sustained throughput and buffer management.
- Method: Send scripted burst (e.g., 100 frames/min) from Mac to rotating arms.
- Acceptance: No buffer overruns; no dropped frames; observed latency within §5 limits.

### AT-008: Malformed Frame Recovery
- Purpose: Ensure resilience to missing terminator.
- Method: Inject frames lacking `##` and then valid frames.
- Acceptance: Malformed frames are discarded after timeout; valid frames thereafter are processed correctly.

### AT-009: Noise and Line Disturbance (Optional)
- Purpose: Characterize behavior under simulated noise.
- Method: Introduce random byte noise between frames (emulator); observe.
- Acceptance: Parser resynchronizes on next `!!` within bounded time; no wedging.

## 7. Measurement, Evidence, and Reporting
- Evidence: USB and UART logs with timestamps; optional logic traces.
- Metrics: Delivery ratio, duplication rate (should be 0), end-to-end latency distributions, recovery time after malformed frames.
- Reporting: Test log with case outcomes, anomalies, and links to artifacts. Defects carry reproduction steps and suspected layer.

## 8. Risk Register (Selected)
- Hardware substitutions may alter UART levels/latency → Validate with each hardware spin.
- Protocol drift across teams → Gate testing on a frozen protocol tag; add compatibility notes when breaking.
- Emulator fidelity vs. real ESP firmware → Test both where possible.

## 9. Schedule and Resourcing (Tentative)
- Bench setup: 0.5 day; Acceptance execution: 1.5 days; Fix/retest: 1 day.
- Roles: QA Lead (coordination), Firmware Engineer (triage/fixes), Systems Integrator (bench + wiring).

## 10. Configuration Management and Sign‑Off
- CM: Reference repo commit, tool versions, and wiring map per test report.
- Sign‑off: QA Lead and Production Engineering approve upon meeting §5.

---

## 11. Integration Plan (Abstract)
This section outlines how the Serial Router will be integrated with the broader system. Details (interfaces, calendars, test data) will be populated as subsystem teams finalize their deliverables. To avoid duplication, this plan references the Acceptance Criteria (§5) and Procedures (§6) instead of redefining tests; integration focuses on sequencing, readiness, and governance.

### 11.1 Objectives
- Establish reliable communication pathways between Mac Mini and all microcontroller endpoints via the router.
- Demonstrate end‑to‑end behaviors required for show operation under nominal and degraded conditions.

### 11.2 Strategy
Adopt a staged, bottom‑up integration with simulation first, then progressive hardware inclusion. At each stage, execute the minimal subset of acceptance tests necessary to demonstrate readiness before advancing:
1. Router Module Validation → Smoke subset of AT-001 (boot/readiness) and parser checks (targeted harness).
2. Mac Link Bring‑Up → AT-001, AT-003 (upstream return path), logging sanity.
3. Single‑Arm Integration → AT-002 (Mac→Arm unicast), AT-004 (auto‑ack, if configured) for the connected arm.
4. Multi‑Arm Expansion → Repeat AT-002 across added arms; confirm isolation (no cross‑talk) consistent with §5.6.
5. Centerpiece Integration → AT-005 (centerpiece addressing) and any timing coordination checks TBD.
6. System Scenarios → Throughput and resilience via AT-007 (throughput/backpressure), AT-008 (malformed recovery), AT-009 (noise, optional).

### 11.3 Entry/Exit per Stage (Abstract)
- Entry: Prior stage closed; interface document baseline approved; hardware available or simulator configured.
- Exit: Stage‑specific checklist complete; no Sev1/Sev2 issues; stage’s referenced Acceptance Tests (see §11.2) passed.

### 11.4 Interface Contracts and Stubs
- Contracts: Message schemas, timing constraints, error codes (per Communication Agreement).
- Stubs/Simulators: Python UART emulators and loopbacks to decouple teams until hardware arrives.

### 11.5 Test Coverage and Data Sources
- Coverage: Integration stages cite Acceptance Tests (§6) as the authoritative procedures; no parallel test lists are maintained here.
- Data sources: Canonical command suites, negative cases, and soak profiles are inherited from Acceptance (§6–§7) and the Communication Agreement; integration does not redefine them.

### 11.6 Fault Injection
Refer to AT-008 (malformed frame recovery) and AT-009 (noise) for procedures and acceptance checks. Integration stages employ these tests only when gating system‑level readiness (typically Stage 6) to avoid redundant execution.

### 11.7 Governance & Reporting
- Weekly integration stand‑up; shared tracker for issues; artifacts archived with run metadata.

### 11.8 Dependencies and Open Items
- Align with final Communication Agreement revision and pin maps.
- Confirm whether GPIO handshakes are required by any subsystem.
- Define soak test duration and acceptance thresholds.

### 11.9 Stage-to-Acceptance Mapping (Guidance)
| Stage | Primary Acceptance Tests |
|---|---|
| Router Module Validation | AT-001 (+ parser harness) |
| Mac Link Bring‑Up | AT-001, AT-003 |
| Single‑Arm Integration | AT-002, AT-004 (if applicable) |
| Multi‑Arm Expansion | AT-002 across all arms; observe §5.6 |
| Centerpiece Integration | AT-005 |
| System Scenarios | AT-007, AT-008, AT-009 (optional) |

---

## Appendices

### Appendix A — Protocol Examples (Informative)
- `!![ARM1]:REQUEST:STATUS##`
- `!!MASTER:REQUEST:MAKE_STAR##` → `!!MASTER:CONFIRM:MAKE_STAR##`
- `!!CENTERPIECE:REQUEST:SYNC##`

### Appendix B — Port and Pin Map (Teensy 4.1)
See §2.3 table. Verify PCB harnessing before powering.

### Appendix C — Tools and Scripts
- `Serial_Router.py` for scripted host traffic.
- ESP sketch `ESP_SerialRouting_Test.ino` for UART bridge and auto‑response behavior.

### Appendix D — Traceability Skeleton (to be completed)
- Map Acceptance Criteria (§5) to clauses in the Communication Agreement and to test cases (§6).

### Appendix E — Glossary
- SUT: System Under Test; CDC: Communications Device Class; UART: Universal Asynchronous Receiver/Transmitter.
