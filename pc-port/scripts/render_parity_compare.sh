#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
pc_port_root="$(cd "${script_dir}/.." && pwd)"
repo_root="$(cd "${pc_port_root}/.." && pwd)"

frame="${SMGPC_PARITY_FRAME:-1900}"
build_mode="${SMGPC_PARITY_XMAKE_MODE:-debug}"
work_dir="${SMGPC_PARITY_WORK_DIR:-${pc_port_root}/.cache/render-parity}"
default_cached_dolphin_png="${pc_port_root}/.cache/dolphin-reference-dump/Frames/framedump_${frame}.png"
dolphin_trace="${SMGPC_PARITY_DOLPHIN_TRACE:-${work_dir}/dolphin-frame-${frame}.trace.ndjson}"
dolphin_png_is_user_supplied=0
dolphin_png_is_cached_reference=0
if [[ -n "${SMGPC_PARITY_DOLPHIN_PNG:-}" ]]; then
  dolphin_png="${SMGPC_PARITY_DOLPHIN_PNG}"
  dolphin_png_is_user_supplied=1
elif [[ -s "${default_cached_dolphin_png}" ]]; then
  dolphin_png="${default_cached_dolphin_png}"
  dolphin_png_is_cached_reference=1
else
  dolphin_png="${work_dir}/dolphin-frame-${dolphin_frame}.png"
fi
pc_png="${SMGPC_PARITY_PC_PNG:-${work_dir}/pcport-frame-${pc_frame}.png}"
pc_trace="${SMGPC_PARITY_PC_TRACE:-${work_dir}/pcport-frame-${pc_frame}.trace.ndjson}"
dolphin_log="${work_dir}/dolphin-frame-${dolphin_frame}.log"
pc_log="${work_dir}/pcport-frame-${pc_frame}.log"
diff_log="${work_dir}/visual-diff-frame-${pc_frame}.log"
trace_sqlite="${SMGPC_PARITY_TRACE_SQLITE:-${work_dir}/traces.sqlite}"
trace_import_log="${work_dir}/trace-sqlite-frame-${pc_frame}.log"
trace_compare_log="${work_dir}/trace-compare-frame-${pc_frame}.log"
manifest_path="${SMGPC_PARITY_MANIFEST:-${work_dir}/manifest.json}"
dolphin_user="${SMGPC_PARITY_DOLPHIN_USER:-${work_dir}/dolphin-user}"
dolphin_shm="${SMGPC_DOLPHIN_SHM_DIR:-${work_dir}/dolphin-shm}"
pc_save_dir_is_default=0
if [[ -n "${SMGPC_PARITY_PC_SAVE_DIR:-}" ]]; then
  pc_save_dir="${SMGPC_PARITY_PC_SAVE_DIR}"
else
  pc_save_dir="${work_dir}/pc-save"
  pc_save_dir_is_default=1
fi
reset_pc_save="${SMGPC_PARITY_RESET_PC_SAVE:-${pc_save_dir_is_default}}"

dolphin_bin="${SMGPC_DOLPHIN_BIN:-${pc_port_root}/dolphin/build-nogui-libcxx/Binaries/dolphin-emu-nogui}"
game_image="${SMGPC_DOLPHIN_GAME:-${repo_root}/Super Mario Wii - Galaxy Adventure (Korea).rvz}"
pc_bin="${SMGPC_PC_BIN:-${pc_port_root}/build/linux/x86_64/${build_mode}/smg-pc}"
visual_diff_bin="${SMGPC_VISUAL_DIFF_BIN:-${pc_port_root}/build/linux/x86_64/${build_mode}/smg-pc-visual-diff}"
trace_import_bin="${SMGPC_TRACE_IMPORT_BIN:-${pc_port_root}/build/linux/x86_64/${build_mode}/smg-pc-trace-import-sqlite}"
trace_compare_bin="${SMGPC_TRACE_COMPARE_BIN:-${pc_port_root}/build/linux/x86_64/${build_mode}/smg-pc-trace-compare-sqlite}"
dolphin_platform="${SMGPC_DOLPHIN_PLATFORM:-x11}"
dolphin_video_backend="${SMGPC_DOLPHIN_VIDEO_BACKEND:-Software}"
timeout_seconds="${SMGPC_PARITY_TIMEOUT_SECONDS:-240}"
max_full_rms="${SMGPC_PARITY_MAX_FULL_NORMALIZED_RMS:-0.35}"
max_crop_rms="${SMGPC_PARITY_MAX_CROP_NORMALIZED_RMS:-}"
crop="${SMGPC_PARITY_CROP:-}"
semantic_category="${SMGPC_PARITY_SEMANTIC_CATEGORY:-capture}"
semantic_name="${SMGPC_PARITY_SEMANTIC_NAME:-requested_frame_${frame}}"
semantic_detail="${SMGPC_PARITY_SEMANTIC_DETAIL:-render_parity_compare frame ${frame}}"

mkdir -p "${work_dir}"

json_escape() {
  local value="${1:-}"
  value="${value//\\/\\\\}"
  value="${value//\"/\\\"}"
  value="${value//$'\n'/\\n}"
  printf '%s' "${value}"
}

json_string() {
  printf '"%s"' "$(json_escape "${1:-}")"
}

file_state() {
  if [[ -s "${1}" ]]; then
    printf 'present'
  else
    printf 'missing'
  fi
}

file_size() {
  if [[ -s "${1}" ]]; then
    stat -c '%s' "${1}"
  else
    printf '0'
  fi
}

write_crop_manifest_entries() {
  local first=1
  IFS=';' read -ra crop_entries <<< "${scenario_crops}"
  for entry in "${crop_entries[@]}"; do
    [[ -n "${entry}" ]] || continue
    local name="${entry%%=*}"
    local rect="${entry#*=}"
    if [[ "${first}" == "0" ]]; then
      printf ',\n'
    fi
    first=0
    printf '    {"name": '
    json_string "${name}"
    printf ', "rect": '
    json_string "${rect}"
    printf '}'
  done
  if [[ "${first}" == "1" ]]; then
    printf '    {"name": "full", "rect": "0,0,640,456"}'
  fi
  printf '\n'
}

write_manifest() {
  local status="${1}"
  local detail="${2:-}"
  {
    printf '{\n'
    printf '  "scenario": '; json_string "${scenario}"; printf ',\n'
    printf '  "scenario_description": '; json_string "${scenario_description}"; printf ',\n'
    printf '  "status": '; json_string "${status}"; printf ',\n'
    printf '  "detail": '; json_string "${detail}"; printf ',\n'
    printf '  "frame": %s,\n' "${frame}"
    printf '  "pc_frame": %s,\n' "${pc_frame}"
    printf '  "dolphin_frame": %s,\n' "${dolphin_frame}"
    printf '  "build_mode": '; json_string "${build_mode}"; printf ',\n'
    printf '  "command": '; json_string "SMGPC_PARITY_SCENARIO=${scenario} ${BASH_SOURCE[0]}"; printf ',\n'
    printf '  "env": {"build": '; json_string "${SMGPC_PARITY_BUILD:-1}"; printf ', "refresh_dolphin": '; json_string "${SMGPC_PARITY_REFRESH_DOLPHIN:-0}"; printf ', "timeout_seconds": '; json_string "${timeout_seconds}"; printf ', "bgfx_renderer": '; json_string "${SMGPC_BGFX_RENDERER:-}"; printf ', "dolphin_platform": '; json_string "${dolphin_platform}"; printf ', "dolphin_video_backend": '; json_string "${dolphin_video_backend}"; printf ', "work_dir": '; json_string "${work_dir}"; printf ', "pc_save_dir": '; json_string "${pc_save_dir}"; printf ', "reset_pc_save": '; json_string "${reset_pc_save}"; printf '},\n'
    printf '  "tools": {"dolphin_bin": '; json_string "${dolphin_bin}"; printf ', "game_image": '; json_string "${game_image}"; printf ', "pc_bin": '; json_string "${pc_bin}"; printf ', "visual_diff_bin": '; json_string "${visual_diff_bin}"; printf ', "trace_import_bin": '; json_string "${trace_import_bin}"; printf ', "trace_compare_bin": '; json_string "${trace_compare_bin}"; printf '},\n'
    printf '  "dolphin_reference_status": '; json_string "${dolphin_reference_status}"; printf ',\n'
    printf '  "thresholds": {"max_full_normalized_rms": '; json_string "${max_full_rms}"; printf ', "max_crop_normalized_rms": '; json_string "${max_crop_rms}"; printf '},\n'
    printf '  "primary_crop": '; json_string "${crop}"; printf ',\n'
    printf '  "crop_definitions": [\n'
    write_crop_manifest_entries
    printf '  ],\n'
    printf '  "semantic_anchor": {"category": '; json_string "${semantic_category}"; printf ', "name": '; json_string "${semantic_name}"; printf ', "detail": '; json_string "${semantic_detail}"; printf '},\n'
    printf '  "dolphin_input": {"button_script": '; json_string "${dolphin_button_script}"; printf ', "pointer_script": '; json_string "${dolphin_pointer_script}"; printf '},\n'
    printf '  "pc_input": {"button_script": '; json_string "${pc_button_script}"; printf ', "pointer_script": '; json_string "${pc_pointer_script}"; printf '},\n'
    printf '  "artifacts": {\n'
    printf '    "dolphin_png": {"path": '; json_string "${dolphin_png}"; printf ', "state": '; json_string "$(file_state "${dolphin_png}")"; printf ', "bytes": %s},\n' "$(file_size "${dolphin_png}")"
    printf '    "dolphin_trace": {"path": '; json_string "${dolphin_trace}"; printf ', "state": '; json_string "$(file_state "${dolphin_trace}")"; printf ', "bytes": %s},\n' "$(file_size "${dolphin_trace}")"
    printf '    "pc_png": {"path": '; json_string "${pc_png}"; printf ', "state": '; json_string "$(file_state "${pc_png}")"; printf ', "bytes": %s},\n' "$(file_size "${pc_png}")"
    printf '    "pc_trace": {"path": '; json_string "${pc_trace}"; printf ', "state": '; json_string "$(file_state "${pc_trace}")"; printf ', "bytes": %s},\n' "$(file_size "${pc_trace}")"
    printf '    "trace_sqlite": {"path": '; json_string "${trace_sqlite}"; printf ', "state": '; json_string "$(file_state "${trace_sqlite}")"; printf ', "bytes": %s},\n' "$(file_size "${trace_sqlite}")"
    printf '    "logs": {"dolphin": '; json_string "${dolphin_log}"; printf ', "pc": '; json_string "${pc_log}"; printf ', "visual_diff": '; json_string "${diff_log}"; printf ', "trace_import": '; json_string "${trace_import_log}"; printf ', "trace_compare": '; json_string "${trace_compare_log}"; printf '}\n'
    printf '  }\n'
    printf '}\n'
  } > "${manifest_path}"
}

validate_artifact_pair() {
  local label="${1}"
  local png_path="${2}"
  local trace_path="${3}"
  local has_png=0
  local has_trace=0
  [[ -s "${png_path}" ]] && has_png=1
  [[ -s "${trace_path}" ]] && has_trace=1
  if [[ "${has_png}" != "${has_trace}" ]]; then
    echo "${label} artifact pair is incomplete: png=$(file_state "${png_path}") trace=$(file_state "${trace_path}")" >&2
    echo "  png: ${png_path}" >&2
    echo "  trace: ${trace_path}" >&2
    return 1
  fi
}

if [[ "${SMGPC_PARITY_DRY_RUN:-0}" == "1" ]]; then
  dolphin_reference_status="dry-run"
  write_manifest "dry-run" "scenario configuration written without build or capture"
  echo "scenario=${scenario}"
  echo "frame=${frame}"
  echo "pc_frame=${pc_frame}"
  echo "dolphin_frame=${dolphin_frame}"
  echo "manifest=${manifest_path}"
  exit 0
fi

mkdir -p "${dolphin_user}" "${dolphin_shm}"
if [[ "${reset_pc_save}" == "1" ]]; then
  rm -rf "${pc_save_dir}"
fi
mkdir -p "${pc_save_dir}"

trap 'rc=$?; if [[ ${rc} -ne 0 ]]; then write_manifest "failed" "exit ${rc}"; fi' EXIT

if [[ "${build_mode}" != "debug" ]]; then
  echo "render_parity_compare.sh requires SMGPC_PARITY_XMAKE_MODE=debug because it builds debug-only trace/visual tools and uses debug-only runtime capture hooks" >&2
  exit 2
fi

if [[ "${build_mode}" != "debug" ]]; then
  echo "render_parity_compare.sh requires SMGPC_PARITY_XMAKE_MODE=debug because it builds debug-only trace/visual tools and uses debug-only runtime capture hooks" >&2
  exit 2
fi

if [[ "${SMGPC_PARITY_BUILD:-1}" == "1" ]]; then
  (cd "${pc_port_root}" && env CC="${CC:-clang-22}" CXX="${CXX:-clang++-22}" xmake f -m "${build_mode}")
  (cd "${pc_port_root}" && env CC="${CC:-clang-22}" CXX="${CXX:-clang++-22}" xmake build smg-pc)
  (cd "${pc_port_root}" && env CC="${CC:-clang-22}" CXX="${CXX:-clang++-22}" xmake build smg-pc-visual-diff)
  (cd "${pc_port_root}" && env CC="${CC:-clang-22}" CXX="${CXX:-clang++-22}" xmake build smg-pc-trace-import-sqlite)
  (cd "${pc_port_root}" && env CC="${CC:-clang-22}" CXX="${CXX:-clang++-22}" xmake build smg-pc-trace-compare-sqlite)
fi

run_dolphin_capture() {
  if [[ "${SMGPC_PARITY_REFRESH_DOLPHIN:-0}" != "1" && -s "${dolphin_png}" && -s "${dolphin_trace}" ]]; then
    if [[ "${dolphin_png_is_user_supplied}" == "1" ]]; then
      dolphin_reference_status="user-supplied"
    elif [[ "${dolphin_png_is_cached_reference}" == "1" ]]; then
      dolphin_reference_status="cached"
    else
      dolphin_reference_status="cached-work-dir"
    fi
    return 0
  fi

  if [[ "${SMGPC_PARITY_REFRESH_DOLPHIN:-0}" != "1" ]]; then
    validate_artifact_pair "Dolphin cached/reference" "${dolphin_png}" "${dolphin_trace}"
  fi

  if [[ ! -x "${dolphin_bin}" ]]; then
    echo "missing Dolphin NoGUI binary: ${dolphin_bin}" >&2
    echo "build it with pc-port/dolphin/build-nogui-libcxx or set SMGPC_DOLPHIN_BIN" >&2
    return 2
  fi
  if ! grep -a -q "SMGPC_DOLPHIN_TRACE_PATH" "${dolphin_bin}"; then
    echo "Dolphin binary does not appear to contain the SMGPC Dolphin trace hooks required for NDJSON parity traces: ${dolphin_bin}" >&2
    echo "provide a patched Dolphin binary via SMGPC_DOLPHIN_BIN or use a complete cached Dolphin PNG+trace pair" >&2
    return 2
  fi
  if [[ ! -f "${game_image}" ]]; then
    echo "missing Dolphin game image: ${game_image}" >&2
    echo "set SMGPC_DOLPHIN_GAME to the Korean SMG RVZ/ISO path" >&2
    return 2
  fi

  if [[ "${dolphin_png_is_cached_reference}" != "1" && ("${dolphin_png_is_user_supplied}" != "1" || "${SMGPC_PARITY_REFRESH_DOLPHIN:-0}" == "1" || ! -s "${dolphin_png}") ]]; then
    rm -f "${dolphin_png}"
  fi
  rm -f "${dolphin_trace}"
  dolphin_reference_status="refreshed"
  (
    env \
      SMGPC_DOLPHIN_SHM_DIR="${dolphin_shm}" \
      SMGPC_DOLPHIN_CAPTURE_FRAME="${dolphin_frame}" \
      SMGPC_DOLPHIN_CAPTURE_PATH="${dolphin_png}" \
      SMGPC_DOLPHIN_TRACE_FRAME="${dolphin_frame}" \
      SMGPC_DOLPHIN_TRACE_PATH="${dolphin_trace}" \
      SMGPC_DOLPHIN_TRACE_WINDOW="${SMGPC_DOLPHIN_TRACE_WINDOW:-0}" \
      SMGPC_DOLPHIN_SEMANTIC_ANCHOR_CATEGORY="${semantic_category}" \
      SMGPC_DOLPHIN_SEMANTIC_ANCHOR_NAME="${semantic_name}" \
      SMGPC_DOLPHIN_SEMANTIC_ANCHOR_DETAIL="${semantic_detail}" \
      xvfb-run -a "${dolphin_bin}" \
        -u "${dolphin_user}" \
        -p "${dolphin_platform}" \
        -v "${dolphin_video_backend}" \
        -C Graphics.Settings.FrameDumpsResolutionType=2 \
        -C Wiimote.Wiimote1.Source=1 \
        -C Wiimote.Wiimote2.Source=0 \
        -C Wiimote.Wiimote3.Source=0 \
        -C Wiimote.Wiimote4.Source=0 \
        -e "${game_image}"
  ) >"${dolphin_log}" 2>&1 &

  local pid="$!"
  local deadline=$((SECONDS + timeout_seconds))
  while (( SECONDS < deadline )); do
    if [[ -s "${dolphin_png}" && -s "${dolphin_trace}" ]]; then
      kill "${pid}" >/dev/null 2>&1 || true
      wait "${pid}" >/dev/null 2>&1 || true
      validate_artifact_pair "Dolphin captured" "${dolphin_png}" "${dolphin_trace}"
      return 0
    fi
    if ! kill -0 "${pid}" >/dev/null 2>&1; then
      break
    fi
    sleep 1
  done

  kill "${pid}" >/dev/null 2>&1 || true
  wait "${pid}" >/dev/null 2>&1 || true
  echo "Dolphin did not write ${dolphin_png} and ${dolphin_trace} within ${timeout_seconds}s" >&2
  tail -80 "${dolphin_log}" >&2 || true
  return 1
}

run_pc_capture() {
  rm -f "${pc_png}" "${pc_trace}"
  local renderer_env=()
  if [[ -n "${SMGPC_BGFX_RENDERER:-}" ]]; then
    renderer_env+=(SMGPC_BGFX_RENDERER="${SMGPC_BGFX_RENDERER}")
  fi
  env \
    "${renderer_env[@]}" \
    SMGPC_ENABLE_VSYNC="${SMGPC_ENABLE_VSYNC:-0}" \
    SMGPC_EVENT_POLL_INTERVAL="${SMGPC_EVENT_POLL_INTERVAL:-8}" \
    SMGPC_ASYNC_SCREENSHOT_PNG="${SMGPC_ASYNC_SCREENSHOT_PNG:-1}" \
    SMGPC_SKIP_RENDER_UNTIL_FRAME="${SMGPC_SKIP_RENDER_UNTIL_FRAME:-${frame}}" \
    SMGPC_WINDOW_WIDTH=640 \
    SMGPC_WINDOW_HEIGHT=456 \
    SMGPC_SCREENSHOT_PATH="${pc_png}" \
    SMGPC_SCREENSHOT_FRAME="${frame}" \
    SMGPC_PARITY_TRACE_PATH="${pc_trace}" \
    SMGPC_PARITY_TRACE_FRAME="${frame}" \
    SMGPC_SEMANTIC_ANCHOR_CATEGORY="${semantic_category}" \
    SMGPC_SEMANTIC_ANCHOR_NAME="${semantic_name}" \
    SMGPC_SEMANTIC_ANCHOR_DETAIL="${semantic_detail}" \
    SMGPC_EXIT_AFTER_SCREENSHOT=1 \
    timeout "${timeout_seconds}s" xvfb-run -a "${pc_bin}" >"${pc_log}" 2>&1

  if [[ ! -s "${pc_png}" ]]; then
    echo "pc-port did not write ${pc_png}" >&2
    tail -80 "${pc_log}" >&2 || true
    return 1
  fi
  if [[ ! -s "${pc_trace}" ]]; then
    echo "pc-port did not write ${pc_trace}" >&2
    tail -80 "${pc_log}" >&2 || true
    return 1
  fi
  validate_artifact_pair "pc-port captured" "${pc_png}" "${pc_trace}"
}

run_dolphin_capture
run_pc_capture

rm -f "${trace_sqlite}"
if ! "${trace_import_bin}" --output "${trace_sqlite}" "${dolphin_trace}" "${pc_trace}" >"${trace_import_log}" 2>&1; then
  echo "trace import failed for scenario ${scenario}" >&2
  tail -80 "${trace_import_log}" >&2 || true
  exit 1
fi
if ! "${trace_compare_bin}" --database "${trace_sqlite}" >"${trace_compare_log}" 2>&1; then
  echo "trace compare failed for scenario ${scenario}" >&2
  tail -80 "${trace_compare_log}" >&2 || true
  exit 1
fi

diff_args=()
if [[ -n "${crop}" ]]; then
  diff_args+=(--crop "${crop}")
fi
if [[ -n "${max_full_rms}" ]]; then
  diff_args+=(--max-full-normalized-rms "${max_full_rms}")
fi
if [[ -n "${max_crop_rms}" ]]; then
  diff_args+=(--max-crop-normalized-rms "${max_crop_rms}")
fi

"${visual_diff_bin}" "${diff_args[@]}" "${dolphin_png}" "${pc_png}" | tee "${diff_log}"

write_manifest "passed" "captured Dolphin/PC artifacts, imported traces, compared trace SQLite, and ran visual diff"
trap - EXIT
