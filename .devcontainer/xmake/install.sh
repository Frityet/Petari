#!/usr/bin/env bash
set -euo pipefail

export DEBIAN_FRONTEND=noninteractive

apt_get_update_if_needed() {
    if [ ! -d "/var/lib/apt/lists" ] || [ "$(ls /var/lib/apt/lists/ | wc -l)" = "0" ]; then
        apt-get update
    fi
}

check_packages() {
    if ! dpkg -s "$@" >/dev/null 2>&1; then
        apt_get_update_if_needed
        apt-get install -y --no-install-recommends "$@"
    fi
}

check_packages \
    build-essential \
    ca-certificates \
    curl \
    git \
    p7zip-full

install_user="${_REMOTE_USER:-${USERNAME:-vscode}}"
if ! id -u "${install_user}" >/dev/null 2>&1; then
    echo "User '${install_user}' does not exist" >&2
    exit 1
fi

if ! command -v xmake >/dev/null 2>&1; then
    case "$(uname -m)" in
        x86_64 | amd64)
            xmake_asset_arch="cosmocc"
            ;;
        *)
            echo "Unsupported architecture for xmake bundle: $(uname -m)" >&2
            exit 1
            ;;
    esac

    xmake_version="${VERSION:-latest}"
    if [ "${xmake_version}" = "latest" ]; then
        xmake_version="$(
            git ls-remote --tags https://github.com/xmake-io/xmake.git 'refs/tags/v*' \
                | awk -F/ '{print $3}' \
                | grep -E '^v[0-9]+[.][0-9]+[.][0-9]+$' \
                | sort -V \
                | tail -n 1
        )"
    elif [ "${xmake_version#v}" = "${xmake_version}" ]; then
        xmake_version="v${xmake_version}"
    fi

    if [ -z "${xmake_version}" ]; then
        echo "Unable to determine latest xmake release version" >&2
        exit 1
    fi

    curl -fsSL \
        "https://github.com/xmake-io/xmake/releases/download/${xmake_version}/xmake-bundle-${xmake_version}.${xmake_asset_arch}" \
        -o /usr/local/bin/xmake
    chmod 755 /usr/local/bin/xmake
fi

if ! command -v xrepo >/dev/null 2>&1; then
    cat >/usr/local/bin/xrepo <<'EOF'
#!/usr/bin/env sh
exec /usr/local/bin/xmake lua private.xrepo "$@"
EOF
    chmod 755 /usr/local/bin/xrepo
fi

su "${install_user}" -c "env XMAKE_COLORTERM=nocolor PATH=/usr/local/bin:/usr/bin:/bin xmake --version"
su "${install_user}" -c "env XMAKE_COLORTERM=nocolor PATH=/usr/local/bin:/usr/bin:/bin xrepo --version"

apt-get clean -y
rm -rf /var/lib/apt/lists/*
