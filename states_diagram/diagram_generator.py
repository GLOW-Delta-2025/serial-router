"""Generate Teensy↔ESP router state diagrams from Markdown specs.

This script parses a structured Markdown file containing diagram metadata,
entities, states, transitions, and command catalogues, then emits a Graphviz
DOT graph (and optionally renders it via the `dot` CLI).

Usage:
	python diagram_generator.py --spec teensy_esp_router_states.md --out output
"""

from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple


@dataclass
class DiagramSpec:
	meta: Dict[str, str]
	entities: List[Dict[str, str]]
	states: List[Dict[str, str]]
	transitions: List[Dict[str, str]]
	commands: List[Dict[str, str]]


SECTION_HEADERS = {"entities", "states", "transitions", "commands", "timing", "notes"}


def parse_front_matter(lines: List[str]) -> Tuple[Dict[str, str], List[str]]:
	if not lines or lines[0].strip() != "---":
		raise ValueError("Spec must begin with YAML-style front matter delimited by `---`.")

	meta: Dict[str, str] = {}
	idx = 1
	while idx < len(lines):
		line = lines[idx].strip()
		if line == "---":
			idx += 1
			break
		if not line or line.startswith("#"):
			idx += 1
			continue
		if ":" not in line:
			raise ValueError(f"Front matter line missing ':' separator: {line}")
		key, value = line.split(":", 1)
		meta[key.strip()] = value.strip()
		idx += 1

	return meta, lines[idx:]


def iter_sections(body_lines: Iterable[str]) -> Dict[str, str]:
	sections: Dict[str, List[str]] = {}
	current_header: Optional[str] = None
	buffer: List[str] = []

	for raw_line in body_lines:
		line = raw_line.rstrip("\n")
		if line.startswith("## "):
			if current_header:
				sections[current_header] = buffer
				buffer = []
			header_name = line[3:].strip().lower()
			if header_name not in SECTION_HEADERS:
				current_header = header_name
				buffer = []
				continue
			current_header = header_name
			buffer = []
		elif current_header:
			buffer.append(line)

	if current_header:
		sections[current_header] = buffer

	return {key: "\n".join(value).strip() for key, value in sections.items() if value}


def parse_markdown_table(section_text: str) -> List[Dict[str, str]]:
	if not section_text:
		return []

	lines = [line.strip() for line in section_text.splitlines() if line.strip()]
	table_lines = [line for line in lines if line.startswith("|") and line.endswith("|")]
	if len(table_lines) < 2:
		return []

	header_tokens = [token.strip() for token in table_lines[0].strip("|").split("|")]
	records: List[Dict[str, str]] = []
	for row in table_lines[2:]:  # skip separator line
		cells = [token.strip() for token in row.strip("|").split("|")]
		if len(cells) != len(header_tokens):
			raise ValueError(f"Row column mismatch in table: {row}")
		records.append(dict(zip(header_tokens, cells)))

	return records


def expand_entity(entity_id: str) -> List[str]:
	range_match = re.fullmatch(r"([A-Za-z]+)(\d+)\.\.(\d+)", entity_id)
	if not range_match:
		return [entity_id]
	prefix, start, end = range_match.groups()
	start_i, end_i = int(start), int(end)
	if end_i < start_i:
		raise ValueError(f"Invalid entity range: {entity_id}")
	return [f"{prefix}{idx}" for idx in range(start_i, end_i + 1)]


def parse_spec(path: Path) -> DiagramSpec:
	lines = path.read_text(encoding="utf-8").splitlines()
	meta, remainder = parse_front_matter(lines)
	sections = iter_sections(remainder)

	entities = parse_markdown_table(sections.get("entities", ""))
	expanded_entities: List[Dict[str, str]] = []
	for entity in entities:
		ids = expand_entity(entity.get("Id", ""))
		for eid in ids:
			expanded = dict(entity)
			expanded["Id"] = eid
			expanded_entities.append(expanded)

	states = parse_markdown_table(sections.get("states", ""))
	transitions = parse_markdown_table(sections.get("transitions", ""))
	commands = parse_markdown_table(sections.get("commands", ""))

	validate_spec(meta, expanded_entities, states, transitions, commands)

	return DiagramSpec(meta=meta, entities=expanded_entities, states=states, transitions=transitions, commands=commands)


def validate_spec(meta: Dict[str, str], entities: List[Dict[str, str]], states: List[Dict[str, str]], transitions: List[Dict[str, str]], commands: List[Dict[str, str]]) -> None:
	if "diagram" not in meta:
		raise ValueError("Front matter missing required 'diagram' identifier.")

	entity_ids = {entity["Id"] for entity in entities}
	state_codes = {state["Code"] for state in states}

	for state in states:
		owner = state.get("Owner", "")
		if owner and owner not in entity_ids and owner not in {"ARM"}:
			raise ValueError(f"Unknown owner '{owner}' for state {state.get('Code')}")

	for transition in transitions:
		src, dst = transition.get("From"), transition.get("To")
		if src not in state_codes:
			raise ValueError(f"Transition references unknown source state: {src}")
		if dst not in state_codes:
			raise ValueError(f"Transition references unknown destination state: {dst}")

	command_ids = {cmd.get("Id") for cmd in commands}
	for transition in transitions:
		message = transition.get("Message", "")
		msg_id = message.split(" ")[0] if message else ""
		if msg_id and msg_id not in command_ids:
			raise ValueError(f"Transition references unknown command: {message}")


def build_color_palette(entities: List[Dict[str, str]]) -> Dict[str, str]:
	palette = ["#5D8AA8", "#F28C28", "#4682B4", "#8F9779", "#B565A7", "#CD5C5C", "#729FCF"]
	assignment: Dict[str, str] = {}
	palette_idx = 0
	for entity in entities:
		eid = entity["Id"]
		if eid not in assignment:
			assignment[eid] = palette[palette_idx % len(palette)]
			palette_idx += 1
	assignment.setdefault("ARM", "#F4C95D")
	return assignment


def sanitise_label(text: str) -> str:
	return text.replace("\n", "\\n").replace("\"", "\\\"")


def render_dot(spec: DiagramSpec) -> str:
	diagram_id = spec.meta.get("diagram", "diagram")
	graph_name = re.sub(r"[^A-Za-z0-9_]", "_", diagram_id)

	colors = build_color_palette(spec.entities)
	command_lookup = {cmd["Id"]: cmd for cmd in spec.commands}

	lines: List[str] = [f"digraph {graph_name} {{", "  rankdir=LR;", "  fontname=Helvetica;", "  node [shape=rectangle, fontname=Helvetica, style=filled];"]

	for state in spec.states:
		code = state["Code"]
		owner = state.get("Owner", "")
		color = colors.get(owner, "#dddddd")
		label = f"{code}\\n{state.get('Name', '')}"
		tooltip_info = {
			"owner": owner,
			"role": state.get("Role", ""),
			"description": state.get("Description", ""),
		}
		tooltip = sanitise_label(json.dumps(tooltip_info, ensure_ascii=False))
		lines.append(f"  \"{code}\" [label=\"{sanitise_label(label)}\", fillcolor=\"{color}\", tooltip=\"{tooltip}\"];\n")

	for transition in spec.transitions:
		src, dst = transition["From"], transition["To"]
		message = transition.get("Message", "")
		notes = transition.get("Notes", "")
		trigger = transition.get("Trigger", "")
		msg_id = message.split(" ")[0] if message else ""
		command = command_lookup.get(msg_id, {})
		edge_label_parts = [part for part in [trigger, message] if part]
		edge_label = "\n".join(edge_label_parts)
		tooltip_payload = {
			"trigger": trigger,
			"message": message,
			"notes": notes,
			"payload": command.get("Payload", ""),
		}
		tooltip = sanitise_label(json.dumps(tooltip_payload, ensure_ascii=False))
		label = sanitise_label(edge_label)
		lines.append(f"  \"{src}\" -> \"{dst}\" [label=\"{label}\", tooltip=\"{tooltip}\"];\n")

	lines.append("}")
	return "\n".join(lines)


def write_outputs(dot_content: str, output_dir: Path, diagram_id: str, image_format: str) -> Path:
	output_dir.mkdir(parents=True, exist_ok=True)
	dot_path = output_dir / f"{diagram_id}.dot"
	dot_path.write_text(dot_content, encoding="utf-8")

	dot_binary = shutil.which("dot")
	if not dot_binary:
		print("[warn] Graphviz 'dot' executable not found. Generated DOT file only.")
		return dot_path

	image_path = output_dir / f"{diagram_id}.{image_format}"
	try:
		subprocess.run(
			[dot_binary, f"-T{image_format}", "-o", str(image_path), str(dot_path)],
			check=True,
		)
		print(f"[info] Rendered diagram: {image_path}")
	except subprocess.CalledProcessError as exc:
		print(f"[error] Graphviz failed: {exc}")
	return dot_path


def main(argv: Optional[List[str]] = None) -> int:
	parser = argparse.ArgumentParser(description="Generate state diagram from Markdown spec.")
	parser.add_argument("--spec", default="states_diagram/teensy_esp_router_states.md", help="Path to Markdown spec file")
	parser.add_argument("--out", default="states_diagram/output", help="Directory to emit DOT/image outputs")
	parser.add_argument("--format", default="svg", help="Image format for Graphviz renderer (svg,png,...)" )
	args = parser.parse_args(argv)

	spec_path = Path(args.spec)
	if not spec_path.exists():
		print(f"[error] Spec file not found: {spec_path}")
		return 1

	try:
		spec = parse_spec(spec_path)
	except Exception as exc:  # pylint: disable=broad-except
		print(f"[error] Failed to parse spec: {exc}")
		return 2

	dot = render_dot(spec)
	write_outputs(dot, Path(args.out), spec.meta["diagram"], args.format)

	print("[info] Generation complete.")
	return 0


if __name__ == "__main__":
	sys.exit(main())
