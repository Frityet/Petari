#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
pc_port_root="$(cd "${script_dir}/.." && pwd)"
repo_root="$(cd "${pc_port_root}/.." && pwd)"

frame="${SMGPC_PARITY_FRAME:-1900}"
build_mode="${SMGPC_PARITY_XMAKE_MODE:-release}"
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
  dolphin_png="${work_dir}/dolphin-frame-${frame}.png"
fi
pc_png="${SMGPC_PARITY_PC_PNG:-${work_dir}/pcport-frame-${frame}.png}"
pc_trace="${SMGPC_PARITY_PC_TRACE:-${work_dir}/pcport-frame-${frame}.trace.ndjson}"
dolphin_log="${work_dir}/dolphin-frame-${frame}.log"
pc_log="${work_dir}/pcport-frame-${frame}.log"
diff_log="${work_dir}/visual-diff-frame-${frame}.log"
trace_sqlite="${SMGPC_PARITY_TRACE_SQLITE:-${work_dir}/traces.sqlite}"
trace_import_log="${work_dir}/trace-sqlite-frame-${frame}.log"
trace_compare_log="${work_dir}/trace-compare-frame-${frame}.log"
dolphin_user="${SMGPC_PARITY_DOLPHIN_USER:-${work_dir}/dolphin-user}"
dolphin_shm="${SMGPC_DOLPHIN_SHM_DIR:-${work_dir}/dolphin-shm}"

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

mkdir -p "${work_dir}" "${dolphin_user}" "${dolphin_shm}"

if [[ "${SMGPC_PARITY_BUILD:-1}" == "1" ]]; then
  (cd "${pc_port_root}" && env CC="${CC:-clang-22}" CXX="${CXX:-clang++-22}" xmake f -m "${build_mode}")
  (cd "${pc_port_root}" && env CC="${CC:-clang-22}" CXX="${CXX:-clang++-22}" xmake build smg-pc)
  (cd "${pc_port_root}" && env CC="${CC:-clang-22}" CXX="${CXX:-clang++-22}" xmake build smg-pc-visual-diff)
  (cd "${pc_port_root}" && env CC="${CC:-clang-22}" CXX="${CXX:-clang++-22}" xmake build smg-pc-trace-import-sqlite)
  (cd "${pc_port_root}" && env CC="${CC:-clang-22}" CXX="${CXX:-clang++-22}" xmake build smg-pc-trace-compare-sqlite)
fi

run_dolphin_capture() {
  if [[ "${SMGPC_PARITY_REFRESH_DOLPHIN:-0}" != "1" && -s "${dolphin_png}" && -s "${dolphin_trace}" ]]; then
    return 0
  fi

  if [[ ! -x "${dolphin_bin}" ]]; then
    echo "missing Dolphin NoGUI binary: ${dolphin_bin}" >&2
    echo "build it with pc-port/dolphin/build-nogui-libcxx or set SMGPC_DOLPHIN_BIN" >&2
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
  (
    env \
      SMGPC_DOLPHIN_SHM_DIR="${dolphin_shm}" \
      SMGPC_DOLPHIN_CAPTURE_FRAME="${frame}" \
      SMGPC_DOLPHIN_CAPTURE_PATH="${dolphin_png}" \
      SMGPC_DOLPHIN_TRACE_FRAME="${frame}" \
      SMGPC_DOLPHIN_TRACE_PATH="${dolphin_trace}" \
      SMGPC_DOLPHIN_TRACE_WINDOW="${SMGPC_DOLPHIN_TRACE_WINDOW:-0}" \
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
}

run_dolphin_capture
run_pc_capture

rm -f "${trace_sqlite}"
"${trace_import_bin}" --output "${trace_sqlite}" "${dolphin_trace}" "${pc_trace}" >"${trace_import_log}" 2>&1
"${trace_compare_bin}" --database "${trace_sqlite}" >"${trace_compare_log}" 2>&1

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
