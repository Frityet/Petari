#!/bin/sh
set -e

apt update
apt install -y p7zip-full curl
su vscode -c "bash -c 'curl -fsSL https://xmake.io/shget.text | bash'"
