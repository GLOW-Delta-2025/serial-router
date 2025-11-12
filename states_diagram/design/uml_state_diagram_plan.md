# UML State Diagram Plan — Teensy Serial Router

Version: 0.1 (Draft)
Date: 2025-11-11

## Purpose
Define the modeling approach to derive a UML (or closely aligned) state machine for the Teensy Serial Router prior to generating the diagram. This plan enumerates states, events, transitions, guards, and actions grounded in the current firmware (`teensy_router.ino`). Stakeholders will validate this plan before the actual diagram artifact is produced.

## Modeling Scope
- Focus: Runtime behavior of message ingestion, parsing, routing, and acknowledgement between Mac (USB Serial) and ESP (Serial1).
- Exclusions: Multi-UART expansion (future), power management, firmware update flows, broadcast aggregation windows, and retry/backoff logic (present in higher-level system diagrams but not implemented in the current router code).

## Notation
- Tooling target: PlantUML (primary) with optional Mermaid export.
- State naming: UpperCamelCase.
- Events: lower_snake_case representing observable triggers (byte_received, frame_complete, buffer_overflow, timeout_elapsed, ack_condition_met).

## Primary States
1. Idle
   - Awaiting data on either stream (no partial frame buffered or partial acceptable).
2. ReceivingMacFrame
   - Accumulating bytes from Mac after detecting '!!'.
3. ReceivingEspFrame
   - Accumulating bytes from ESP after detecting '!!'.
4. ParsingFrame
   - Validating structure, tokenizing fields, extracting parameters.
5. Routing
   - Determining destination path and injecting/transforming addresses (e.g., prepend MASTER).
6. ForwardingToEsp
   - Writing encoded frame to Serial1.
7. ForwardingToMac
   - Writing encoded frame to USB Serial.
8. AcknowledgingEsp
   - Building and sending CONFIRM frame to ESP based on command pattern.
9. ErrorRecovery
   - Handling malformed frame, overflow, or timeout until resynchronization.

(Note: Forwarding/Acknowledging can also be modeled as internal transitions with actions; separated here for observability and potential timing metrics.)

## Events
- usb_byte_received
- esp_byte_received
- frame_complete
- malformed_frame_detected
- buffer_overflow
- ack_condition_met (for commands requiring router confirmations: STAR_ARRIVED, CLIMAX_READY, COMM_ERROR)
- timeout_elapsed (incomplete frame over threshold)
- parse_failure
- forward_success

## Guards
- is_mac_stream / is_esp_stream
- has_terminator (found '##')
- buffer_within_limit
- command_requires_ack (true when command ∈ {STAR_ARRIVED, CLIMAX_READY, COMM_ERROR})
- device_resolved (pending table hit)

## Actions
- append_byte(buffer)
- discard_until_sync (drop leading bytes until next '!!')
- parse_tokens
- build_outbound_frame
- enqueue_to_serial (USB or UART)
- update_pending(command, device)
- send_ack(command, status)
- log_event(level, message)
- clear_partial(buffer)
- start_timeout_timer(stream)
- purge_stale_pending_entries

## Transition Outline (Abstract)
- Idle --usb_byte_received(with '!!')--> ReceivingMacFrame / start_timeout_timer
- Idle --esp_byte_received(with '!!')--> ReceivingEspFrame / start_timeout_timer
- ReceivingMacFrame --usb_byte_received(no terminator yet)--> ReceivingMacFrame / append_byte
- ReceivingEspFrame --esp_byte_received(no terminator yet)--> ReceivingEspFrame / append_byte
- Receiving*Frame --byte_received('##' completes)--> ParsingFrame / append_byte
- ParsingFrame --parse_success(mac frame & REQUEST)--> Routing
- ParsingFrame --parse_failure--> ErrorRecovery / log_event
- Routing --destination_is_esp--> ForwardingToEsp / build_outbound_frame
- Routing --destination_is_mac--> ForwardingToMac / build_outbound_frame
- ForwardingToEsp --forward_success & command_requires_ack--> AcknowledgingEsp / send_ack
- ForwardingToEsp --forward_success & !command_requires_ack--> Idle
- ForwardingToMac --forward_success--> Idle
- AcknowledgingEsp --forward_success--> Idle
- Any Receiving*Frame --timeout_elapsed--> ErrorRecovery / clear_partial
- ErrorRecovery --discard_until_sync found--> Idle
- Any State --buffer_overflow--> ErrorRecovery / clear_partial

## Derived Timing Points
- T1: first sync bytes ('!!') seen
- T2: frame_complete
- T3: parse_success
- T4: forward_success
- T5: ack_sent
(Used to compute parsing latency (T2→T3), routing latency (T3→T4), round-trip ack latency (T1→T5)).

## Instrumentation Plan
- Add optional macros in firmware to emit timestamps at T1..T5.
- Expose counts: frames_routed, frames_malformed, bytes_dropped, acks_sent.

## Open Questions
- Should ForwardingToEsp and AcknowledgingEsp collapse into a single atomic action? (Depends on whether future retries/backpressure are modeled.)
- Introduce separate state for BufferingWithoutSync to differentiate pre-sync noise vs. active frame?
- Add state for CorrelatingResponse if multi-device pending logic expands.

## Next Steps
1. Review and approve state list, events, transitions.
2. Confirm any additional guards (e.g., rate limiting) before diagramming.
3. Generate PlantUML diagram file (planned: `teensy_router_states.puml`).
4. Iterate after embedding instrumentation in firmware to validate state timing assumptions.

## Consistency with Prior Diagram (teensy_esp_router_states_v2)
- Alignment
   - Source tagging and routing: New plan models address rewriting (prepend MASTER) and forwarding in Routing/Forwarding* states, matching v2 notes and current code behavior.
   - Router confirmations: AcknowledgingEsp state covers CONFIRM emissions for STAR_ARRIVED and CLIMAX_READY, and acknowledges COMM_ERROR, as described in v2 and implemented in `sendAckToEsp`.
   - Error surfacing: ErrorRecovery state plus upstream forwarding reflects v2’s COMM_ERROR reporting/escalation.
- Intentional scope deltas
   - Command-level choreography (MAKE_STAR, SEND_STAR, ADD_STAR, broadcasts START/STOP_IDLE, PING, RESET) in v2 is a higher-level system view. This plan models the transport-layer router FSM; those command states are not enumerated here.
   - Broadcast aggregation and retry/backoff timers in v2 are excluded pending implementation in firmware; placeholders can be added in a future revision.
   - Multi-UART fan-out (ARMs 1..5, CENTER/TOP) is summarized but not explicitly modeled per-port; current code routes via Serial1 only.
  
Conclusion: Within its defined scope (transport-level routing and acknowledgement), the new plan is consistent with the prior v2 diagram and the current firmware. Features unique to v2 (broadcast aggregation, retries, multi-UART fan-out) are noted as future extensions.
