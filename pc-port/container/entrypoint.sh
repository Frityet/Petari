#!/usr/bin/env sh
set -eu

if [ "$#" -eq 0 ]; then
    if [ -t 0 ] && [ -t 1 ]; then
        exec /bin/zsh -l
    fi

    cat <<'EOF'
pc-port container image

Start an interactive shell:
  podman run --rm -it pc-port:podman-test

Start an interactive shell with this repo mounted read/write:
  podman run --rm -it --userns=keep-id --security-opt label=disable -v "$PWD:/workspaces/pcport/pc-port" pc-port:podman-test

Run smg-pc with X11 display access:
  podman run --rm --userns=keep-id --ipc=host --security-opt label=disable -e DISPLAY -e XAUTHORITY=/tmp/.container-xauth -v "$PWD:/workspaces/pcport/pc-port" -v /tmp/.X11-unix:/tmp/.X11-unix:ro -v "$XAUTHORITY:/tmp/.container-xauth:ro" pc-port:podman-test xmake run smg-pc

Run a command:
  podman run --rm pc-port:podman-test xmake --version
EOF
    exit 0
fi

exec "$@"
