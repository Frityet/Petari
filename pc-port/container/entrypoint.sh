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

Run a command:
  podman run --rm pc-port:podman-test xmake --version
EOF
    exit 0
fi

exec "$@"
