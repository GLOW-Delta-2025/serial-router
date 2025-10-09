---
diagram: teensy-esp-router
version: 0.2.0
updated: 2025-10-09
author: GLOW Systems Team
description: Teensy router orchestration covering MAC, arms, centerpiece, top piece, and broadcast control loops.
---

## Entities
| Id | Label | Type | Description |
| --- | --- | --- | --- |
| MAC | Mac Mini | producer | Originates choreography requests and handles confirmations |
| ROUTER | Teensy 4.1 Router | router | Inserts source IDs, routes frames, enforces retries, and aggregates broadcasts |
| ARM1..5 | Arm Controllers | consumer | Build stars from microphone input and report lifecycle updates |
| CENTER | Centerpiece Controller | consumer | Buffers stars, manages buildup, and drives centerpiece climax actions |
| TOP | Top Piece Controller | consumer | Executes launch sequence after centerpiece release |

## States
| Code | Name | Owner | Role | Description |
| --- | --- | --- | --- | --- |
| S_IDLE_ROUTER | ROUTER_IDLE | ROUTER | neutral | Router ready to route frames and track outstanding exchanges |
| S_REQ_101_MAKE_STAR | REQUEST_MAKE_STAR | MAC | request | MAC instructs the addressed arm to begin capturing microphone data for a star |
| S_CONF_201_MAKE_STAR | CONFIRM_MAKE_STAR | ARM | confirm | Arm confirms it has entered star buildup mode and is sampling parameters |
| S_REQ_102_SEND_STAR | REQUEST_SEND_STAR | MAC | request | MAC orders the arm to stream the finalized star payload toward the router |
| S_CONF_202_SEND_STAR | CONFIRM_SEND_STAR | ARM | confirm | Arm acknowledges SEND_STAR and starts transmission on its UART link |
| S_REQ_103_CANCEL_STAR | REQUEST_CANCEL_STAR | MAC | request | MAC aborts a pending star buildup on the specified arm |
| S_CONF_203_CANCEL_STAR | CONFIRM_CANCEL_STAR | ARM | confirm | Arm reports cancellation completed and local buffers cleared |
| S_REQ_104_STAR_ARRIVED | REPORT_STAR_ARRIVED | ARM | notify | Arm reports that its star payload reached the centerpiece ingress queue |
| S_REQ_105_ADD_STAR | REQUEST_ADD_STAR | MAC | request | MAC forwards ADD_STAR toward CENTER or TOP after router rewrites addressing |
| S_CONF_205_ADD_STAR | CONFIRM_ADD_STAR | CENTER | confirm | Center or Top confirms the star payload has been stored safely |
| S_REQ_106_BUILDUP_CLIMAX_CENTER | REQUEST_BUILDUP_CLIMAX_CENTER | MAC | request | MAC accelerates centerpiece buildup animation tempo |
| S_CONF_206_BUILDUP_CLIMAX_CENTER | CONFIRM_BUILDUP_CLIMAX_CENTER | CENTER | confirm | Centerpiece acknowledges the buildup speed change |
| S_REQ_107_CLIMAX_READY | REPORT_CLIMAX_READY | CENTER | notify | Centerpiece announces that the climax threshold has been reached |
| S_REQ_108_START_CLIMAX_CENTER | REQUEST_START_CLIMAX_CENTER | MAC | request | MAC authorizes the centerpiece to eject queued stars toward the top piece |
| S_CONF_208_START_CLIMAX_CENTER | CONFIRM_START_CLIMAX_CENTER | CENTER | confirm | Centerpiece confirms it has started ejecting stars during climax |
| S_REQ_109_START_CLIMAX_TOP | REQUEST_START_CLIMAX_TOP | MAC | request | MAC commands the top piece to start its launch choreography |
| S_CONF_209_START_CLIMAX_TOP | CONFIRM_START_CLIMAX_TOP | TOP | confirm | Top piece confirms the climax launch sequence is running |
| S_REQ_110_STOP_CLIMAX_CENTER | REQUEST_STOP_CLIMAX_CENTER | MAC | request | MAC issues a manual stop for the centerpiece climax motion |
| S_CONF_210_STOP_CLIMAX_CENTER | CONFIRM_STOP_CLIMAX_CENTER | CENTER | confirm | Centerpiece reports it halted the climax routine |
| S_REQ_111_STOP_CLIMAX_TOP | REQUEST_STOP_CLIMAX_TOP | MAC | request | MAC issues a manual stop for the top piece launch |
| S_CONF_211_STOP_CLIMAX_TOP | CONFIRM_STOP_CLIMAX_TOP | TOP | confirm | Top piece reports it halted the launch routine |
| S_REQ_301_START_IDLE | BROADCAST_START_IDLE | MAC | broadcast | MAC sends START_IDLE broadcast through the router to every device |
| S_CONF_401_START_IDLE | CONFIRM_START_IDLE | ROUTER | confirm | Router aggregates field confirmations that idle mode engaged |
| S_REQ_302_STOP_IDLE | BROADCAST_STOP_IDLE | MAC | broadcast | MAC sends STOP_IDLE broadcast to end the idle animation |
| S_CONF_402_STOP_IDLE | CONFIRM_STOP_IDLE | ROUTER | confirm | Router aggregates confirmations that idle mode ended |
| S_REQ_303_PING | BROADCAST_PING | MAC | broadcast | MAC sends heartbeat PING broadcast for liveness checks |
| S_CONF_403_PING | CONFIRM_PING | ROUTER | confirm | Router collects alive responses from all devices |
| S_REQ_304_RESET | BROADCAST_RESET | MAC | broadcast | MAC triggers an emergency RESET broadcast across the bus |
| S_CONF_404_RESET | CONFIRM_RESET | ROUTER | confirm | Router aggregates reset confirmations before resuming routing |
| S_ERR_500_COMM_ERROR | COMM_ERROR | ROUTER | error | Router surfaces communication errors or timeouts to the MAC for recovery |

## Transitions
| From | Trigger | Message | To | Notes |
| --- | --- | --- | --- | --- |
| S_IDLE_ROUTER | mic_start_event | ID101 MAKE_STAR | S_REQ_101_MAKE_STAR | MAC selects target arm; router will append MASTER source when forwarding |
| S_REQ_101_MAKE_STAR | arm_ack_started | ID201 MAKE_STAR_CONFIRM | S_CONF_201_MAKE_STAR | Router tags arm reply with source before relaying to MAC |
| S_CONF_201_MAKE_STAR | mic_session_complete | ID102 SEND_STAR | S_REQ_102_SEND_STAR | MAC closes capture window and requests payload transfer |
| S_REQ_102_SEND_STAR | arm_ack_payload | ID202 SEND_STAR_CONFIRM | S_CONF_202_SEND_STAR | Router holds context until arm confirms transmission start |
| S_CONF_202_SEND_STAR | router_dispatch_to_center | ID105 ADD_STAR | S_REQ_105_ADD_STAR | Router rewrites destination to CENTER or TOP and forwards bulk payload |
| S_CONF_202_SEND_STAR | arm_status_forward | ID104 STAR_ARRIVED | S_REQ_104_STAR_ARRIVED | Router pushes arm perspective update upstream to MAC |
| S_REQ_104_STAR_ARRIVED | router_autoroute |  | S_REQ_105_ADD_STAR | Status update primes MAC to ensure ADD_STAR fan-out continues |
| S_REQ_105_ADD_STAR | center_ack_buffered | ID205 ADD_STAR_CONFIRM | S_CONF_205_ADD_STAR | Centerpiece confirms storage; router clears pending payload flag |
| S_CONF_205_ADD_STAR | mac_drives_buildup | ID106 BUILDUP_CLIMAX_CENTER | S_REQ_106_BUILDUP_CLIMAX_CENTER | MAC advances buildup pacing once minimum stars arrive |
| S_REQ_106_BUILDUP_CLIMAX_CENTER | center_ack_buildup | ID206 BUILDUP_CLIMAX_CENTER_CONFIRM | S_CONF_206_BUILDUP_CLIMAX_CENTER | Centerpiece confirms new buildup timing |
| S_CONF_206_BUILDUP_CLIMAX_CENTER | center_threshold_met | ID107 CLIMAX_READY | S_REQ_107_CLIMAX_READY | Centerpiece notifies MAC that climax conditions are satisfied |
| S_REQ_107_CLIMAX_READY | mac_authorizes_climax | ID108 START_CLIMAX_CENTER | S_REQ_108_START_CLIMAX_CENTER | MAC grants centerpiece permission to start ejecting stars |
| S_REQ_108_START_CLIMAX_CENTER | center_ack_climax | ID208 START_CLIMAX_CENTER_CONFIRM | S_CONF_208_START_CLIMAX_CENTER | Centerpiece confirms climax execution |
| S_CONF_208_START_CLIMAX_CENTER | mac_orchestrates_top | ID109 START_CLIMAX_TOP | S_REQ_109_START_CLIMAX_TOP | Router retargets command to TOP with MASTER source header |
| S_REQ_109_START_CLIMAX_TOP | top_ack_launch | ID209 START_CLIMAX_TOP_CONFIRM | S_CONF_209_START_CLIMAX_TOP | Top piece confirms launch sequence |
| S_CONF_209_START_CLIMAX_TOP | mac_manual_override | ID110 STOP_CLIMAX_CENTER | S_REQ_110_STOP_CLIMAX_CENTER | MAC issues center stop when manual override is triggered |
| S_REQ_110_STOP_CLIMAX_CENTER | center_ack_stop | ID210 STOP_CLIMAX_CENTER_CONFIRM | S_CONF_210_STOP_CLIMAX_CENTER | Centerpiece halts climax operations |
| S_CONF_210_STOP_CLIMAX_CENTER | mac_followup_stop | ID111 STOP_CLIMAX_TOP | S_REQ_111_STOP_CLIMAX_TOP | MAC cascades stop to the top piece |
| S_REQ_111_STOP_CLIMAX_TOP | top_ack_stop | ID211 STOP_CLIMAX_TOP_CONFIRM | S_CONF_211_STOP_CLIMAX_TOP | Top piece confirms stop |
| S_CONF_211_STOP_CLIMAX_TOP | router_clear_context |  | S_IDLE_ROUTER | Router releases climax state and returns to idle |
| S_REQ_101_MAKE_STAR | mac_cancel_during_capture | ID103 CANCEL_STAR | S_REQ_103_CANCEL_STAR | MAC aborts star creation while capture is ongoing |
| S_REQ_102_SEND_STAR | mac_cancel_during_send | ID103 CANCEL_STAR | S_REQ_103_CANCEL_STAR | MAC aborts star creation while payload is in flight |
| S_REQ_103_CANCEL_STAR | arm_ack_cancel | ID203 CANCEL_STAR_CONFIRM | S_CONF_203_CANCEL_STAR | Arm confirms cancellation |
| S_CONF_203_CANCEL_STAR | router_drop_context |  | S_IDLE_ROUTER | Router clears cancelled star context |
| S_IDLE_ROUTER | manual_idle_enable | ID301 START_IDLE | S_REQ_301_START_IDLE | MAC enables idle playback across all devices |
| S_REQ_301_START_IDLE | devices_ack_idle | ID401 START_IDLE_CONFIRM | S_CONF_401_START_IDLE | Router aggregates START_IDLE confirms |
| S_CONF_401_START_IDLE | router_idle_complete |  | S_IDLE_ROUTER | Router returns to idle after broadcast completion |
| S_IDLE_ROUTER | manual_idle_disable | ID302 STOP_IDLE | S_REQ_302_STOP_IDLE | MAC stops idle playback across all devices |
| S_REQ_302_STOP_IDLE | devices_ack_stop_idle | ID402 STOP_IDLE_CONFIRM | S_CONF_402_STOP_IDLE | Router aggregates STOP_IDLE confirms |
| S_CONF_402_STOP_IDLE | router_resume_active |  | S_IDLE_ROUTER | Router returns to idle after idle-stop broadcast |
| S_IDLE_ROUTER | heartbeat_tick | ID303 PING | S_REQ_303_PING | MAC sends heartbeat ping |
| S_REQ_303_PING | devices_echo_ping | ID403 PING_CONFIRM | S_CONF_403_PING | Router collects ping replies |
| S_CONF_403_PING | heartbeat_complete |  | S_IDLE_ROUTER | Router returns to idle after heartbeat |
| S_IDLE_ROUTER | operator_reset | ID304 RESET | S_REQ_304_RESET | MAC issues emergency reset broadcast |
| S_REQ_304_RESET | devices_ack_reset | ID404 RESET_CONFIRM | S_CONF_404_RESET | Router aggregates reset confirmations |
| S_CONF_404_RESET | router_reset_complete |  | S_IDLE_ROUTER | Router resumes idle after reset loop |
| S_REQ_102_SEND_STAR | router_timeout | ID500 COMM_ERROR | S_ERR_500_COMM_ERROR | Router escalates timeout while waiting for SEND_STAR payload |
| S_REQ_105_ADD_STAR | center_timeout | ID500 COMM_ERROR | S_ERR_500_COMM_ERROR | Router escalates timeout waiting for center or top ACK |
| S_ERR_500_COMM_ERROR | mac_recovery_reset |  | S_REQ_304_RESET | MAC responds to COMM_ERROR by issuing broadcast RESET |

## Commands
| Id | Label | Direction | Payload | Description |
| --- | --- | --- | --- | --- |
| ID101 | MAKE_STAR | MAC→ROUTER→ARM | arm_id,speed,color,brightness,size | Start star capture on a specific arm with live mic parameters |
| ID102 | SEND_STAR | MAC→ROUTER→ARM | arm_id | Request the arm to transmit its completed star payload |
| ID103 | CANCEL_STAR | MAC→ROUTER→ARM | arm_id | Abort the active star capture on an arm |
| ID104 | STAR_ARRIVED | ARM→ROUTER→MAC | arm_id,speed,color,brightness,size | Arm reports its star entered the centerpiece queue |
| ID105 | ADD_STAR | MAC→ROUTER→CENTER/TOP | device_id,star_payload | Forward a star payload into the CENTER or TOP buffer |
| ID106 | BUILDUP_CLIMAX_CENTER | MAC→ROUTER→CENTER | buildup_time_ms | Increase centerpiece buildup timing profile |
| ID107 | CLIMAX_READY | CENTER→ROUTER→MAC | star_count,energy_level | Centerpiece signals it is ready to trigger the climax |
| ID108 | START_CLIMAX_CENTER | MAC→ROUTER→CENTER | climax_time_ms | Authorize centerpiece climax ejection |
| ID109 | START_CLIMAX_TOP | MAC→ROUTER→TOP | climax_time_ms | Command top piece to start the launch routine |
| ID110 | STOP_CLIMAX_CENTER | MAC→ROUTER→CENTER | reason | Manual stop request for centerpiece climax sequence |
| ID111 | STOP_CLIMAX_TOP | MAC→ROUTER→TOP | reason | Manual stop request for top piece climax sequence |
| ID201 | MAKE_STAR_CONFIRM | ARM→ROUTER→MAC | request_id,status | Arm acknowledges MAKE_STAR |
| ID202 | SEND_STAR_CONFIRM | ARM→ROUTER→MAC | request_id,status | Arm acknowledges SEND_STAR |
| ID203 | CANCEL_STAR_CONFIRM | ARM→ROUTER→MAC | request_id,status | Arm confirms star capture cancelled |
| ID205 | ADD_STAR_CONFIRM | CENTER/TOP→ROUTER→MAC | request_id,status | Center or top confirms star buffered |
| ID206 | BUILDUP_CLIMAX_CENTER_CONFIRM | CENTER→ROUTER→MAC | request_id,status | Centerpiece confirms buildup adjustment |
| ID208 | START_CLIMAX_CENTER_CONFIRM | CENTER→ROUTER→MAC | request_id,status | Centerpiece confirms climax sequence started |
| ID209 | START_CLIMAX_TOP_CONFIRM | TOP→ROUTER→MAC | request_id,status | Top piece confirms climax launch started |
| ID210 | STOP_CLIMAX_CENTER_CONFIRM | CENTER→ROUTER→MAC | request_id,status | Centerpiece confirms climax stopped |
| ID211 | STOP_CLIMAX_TOP_CONFIRM | TOP→ROUTER→MAC | request_id,status | Top piece confirms climax stopped |
| ID301 | START_IDLE | MAC→ROUTER→BROADCAST | mode | Broadcast entry to idle animation |
| ID302 | STOP_IDLE | MAC→ROUTER→BROADCAST | mode | Broadcast exit from idle animation |
| ID303 | PING | MAC→ROUTER→BROADCAST | timestamp | Broadcast heartbeat ping |
| ID304 | RESET | MAC→ROUTER→BROADCAST | reason | Broadcast emergency reset |
| ID401 | START_IDLE_CONFIRM | DEVICE→ROUTER→MAC | device_id,status | Device confirms idle mode started |
| ID402 | STOP_IDLE_CONFIRM | DEVICE→ROUTER→MAC | device_id,status | Device confirms idle mode stopped |
| ID403 | PING_CONFIRM | DEVICE→ROUTER→MAC | device_id,status | Device responds to ping heartbeat |
| ID404 | RESET_CONFIRM | DEVICE→ROUTER→MAC | device_id,status | Device acknowledges reset broadcast |
| ID500 | COMM_ERROR | DEVICE→ROUTER→MAC | device_id,error_code,error_text | Router reports communication failure for escalation |

## Timing
- router_retry_limit: 3
- router_retry_backoff_ms: 250
- broadcast_ack_window_ms: 750
- heartbeat_interval_ms: 2000
- error_escalation_timeout_ms: 1500

## Notes
- Router prepends the true source identifier before forwarding any outbound command and strips it when relaying confirmations.
- Broadcast confirmations fan-in at the router; the MAC proceeds only after all targeted devices report success or retries expire.
- COMM_ERROR routes always trigger a RESET broadcast unless a higher level handler preempts with a different recovery action.
- Arms, centerpiece, and top piece share the same framing, so device_id values map to UART port indices for traceability.

