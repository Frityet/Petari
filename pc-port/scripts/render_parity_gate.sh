#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
pc_port_root="$(cd "${script_dir}/.." && pwd)"

default_scenarios=(
  title_wait
  title_decide
  file_select_far
  file_confirm_near
  picturebook_page1
  picturebook_wait
)

if [[ "$#" -gt 0 ]]; then
  scenarios=("$@")
elif [[ -n "${SMGPC_PARITY_GATE_SCENARIOS:-}" ]]; then
  read -r -a scenarios <<< "${SMGPC_PARITY_GATE_SCENARIOS}"
else
  scenarios=("${default_scenarios[@]}")
fi

work_root="${SMGPC_PARITY_GATE_WORK_DIR:-${pc_port_root}/.cache/render-parity-gate/latest}"
aggregate_manifest="${SMGPC_PARITY_GATE_MANIFEST:-${work_root}/manifest.json}"
mkdir -p "${work_root}"

gate_build="${SMGPC_PARITY_BUILD:-1}"
scenario_manifests=()

for index in "${!scenarios[@]}"; do
  scenario="${scenarios[${index}]}"
  scenario_work_dir="${work_root}/${scenario}"
  mkdir -p "${scenario_work_dir}"

  scenario_build="${gate_build}"
  if [[ "${index}" != "0" && "${gate_build}" == "1" ]]; then
    scenario_build="0"
  fi

  echo "render-parity-gate: ${scenario}"
  env \
    SMGPC_PARITY_BUILD="${scenario_build}" \
    SMGPC_PARITY_WORK_DIR="${scenario_work_dir}" \
    "${script_dir}/render_parity_compare.sh" "${scenario}"

  scenario_manifests+=("${scenario_work_dir}/manifest.json")
done

python3 - "${aggregate_manifest}" "${scenario_manifests[@]}" <<'PY'
import json
import sys
from pathlib import Path

out_path = Path(sys.argv[1])
entries = []
overall = "passed"
for raw_path in sys.argv[2:]:
    path = Path(raw_path)
    manifest = json.loads(path.read_text())
    status = manifest.get("status", "missing")
    if status != "passed":
        overall = "failed"
    entries.append({
        "scenario": manifest.get("scenario", path.parent.name),
        "status": status,
        "pc_frame": manifest.get("pc_frame", manifest.get("frame")),
        "dolphin_frame": manifest.get("dolphin_frame", manifest.get("frame")),
        "manifest": str(path),
        "visual_diff": manifest.get("artifacts", {}).get("logs", {}).get("visual_diff", ""),
        "trace_compare": manifest.get("artifacts", {}).get("logs", {}).get("trace_compare", ""),
    })

out_path.parent.mkdir(parents=True, exist_ok=True)
out_path.write_text(json.dumps({
    "status": overall,
    "scenario_count": len(entries),
    "scenarios": entries,
}, indent=2) + "\n")

print(f"render-parity-gate: {overall} {out_path}")
if overall != "passed":
    sys.exit(1)
PY
