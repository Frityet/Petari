#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

xmake f -y -P "${ROOT_DIR}/pc-port"
xmake b -y -P "${ROOT_DIR}/pc-port"

exec xmake run -y -P "${ROOT_DIR}/pc-port" -w "${ROOT_DIR}" pc-port --game-root "${ROOT_DIR}" "$@"
