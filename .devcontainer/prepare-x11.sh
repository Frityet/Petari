#!/usr/bin/env bash
set -euo pipefail

if [ -n "${DISPLAY:-}" ] && command -v xhost >/dev/null 2>&1; then
    xhost +local: >/dev/null 2>&1 || true
fi

display="${DISPLAY:-}"
display="${display#unix:}"
case "${display}" in
    :*) ;;
    *) exit 0 ;;
esac

display_number="${display#:}"
display_number="${display_number%%.*}"
case "${display_number}" in
    ''|*[!0-9]*) exit 0 ;;
esac

display_socket="/tmp/.X11-unix/X${display_number}"
if [ -S "${display_socket}" ]; then
    if DISPLAY=":${display_number}" xhost >/dev/null 2>&1; then
        exit 0
    fi
    if [ ! -L "${display_socket}" ]; then
        exit 0
    fi
fi
if [ -L "${display_socket}" ]; then
    rm -f "${display_socket}"
fi
if [ -e "${display_socket}" ]; then
    exit 0
fi

candidate=""
for socket in /tmp/.X11-unix/X*; do
    [ -S "${socket}" ] || continue
    socket_number="${socket##*/X}"
    case "${socket_number}" in
        ''|*[!0-9]*) continue ;;
    esac
    if DISPLAY=":${socket_number}" xhost >/dev/null 2>&1; then
        candidate="${socket}"
        break
    fi
done

if [ -n "${candidate}" ]; then
    ln -s "$(basename "${candidate}")" "${display_socket}" 2>/dev/null || true
fi
