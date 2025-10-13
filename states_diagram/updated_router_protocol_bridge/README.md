# Updated Router Protocol Diagram

This directory tracks the state-machine documentation for the refreshed Teensy↔ESP router firmware in `actual_code/updated_router_protocol_bridge/`.

## Regenerate the Diagram

```bash
python3 ../diagram_generator.py \
  --spec states_diagram/updated_router_protocol_bridge/teensy_esp_router_states_v2.md \
  --out states_diagram/updated_router_protocol_bridge/output
```

The command writes an updated DOT file (and, when Graphviz is installed, an SVG) that reflects the extended command coverage and router acknowledgements added in the latest firmware.
