#!/usr/bin/env bash

set -euo pipefail

if ! launcher_path="$(readlink -f -- "${BASH_SOURCE[0]}")"; then
    printf 'error: could not resolve launcher path: %s\n' "${BASH_SOURCE[0]}" >&2
    exit 1
fi

launcher_dir="$(dirname -- "$launcher_path")"
showcase_binary="$launcher_dir/bin/smg-pc-showcase"

if (( $# > 1 )); then
    printf 'usage: %s [DISC_IMAGE]\n' "${0##*/}" >&2
    printf '       or set SMGPC_DISC_IMAGE\n' >&2
    exit 2
fi

disc_image="${1:-${SMGPC_DISC_IMAGE:-}}"
if [[ -z "$disc_image" ]]; then
    printf 'error: pass a disc image as the first argument or set SMGPC_DISC_IMAGE\n' >&2
    exit 2
fi

if [[ ! -f "$disc_image" || ! -r "$disc_image" ]]; then
    printf 'error: disc image is not a readable file: %s\n' "$disc_image" >&2
    exit 2
fi

if [[ ! -x "$showcase_binary" ]]; then
    printf 'error: packaged showcase executable is missing or not executable: %s\n' \
        "$showcase_binary" >&2
    exit 1
fi

exec "$showcase_binary" title --disc "$disc_image"
