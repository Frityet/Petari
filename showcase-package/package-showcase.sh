#!/usr/bin/env bash

set -euo pipefail

usage() {
    printf 'usage: %s SHOWCASE_BINARY DESTINATION_DIRECTORY\n' "${0##*/}" >&2
}

die() {
    printf 'error: %s\n' "$1" >&2
    exit 1
}

worktree_state() {
    local repository="$1"
    if [[ -n "$(git -C "$repository" status --porcelain --untracked-files=normal)" ]]; then
        printf 'dirty'
    else
        printf 'clean'
    fi
}

if (( $# != 2 )); then
    usage
    exit 2
fi

if ! script_path="$(readlink -f -- "${BASH_SOURCE[0]}")"; then
    die "could not resolve packaging script path: ${BASH_SOURCE[0]}"
fi

script_dir="$(dirname -- "$script_path")"
project_dir="$(cd -- "$script_dir/.." && pwd -P)"
source_git_dir="$(git -C "$project_dir" rev-parse --show-toplevel)" || \
    die "could not find the source Git worktree"
aurora_git_dir="$project_dir/aurora"

git -C "$aurora_git_dir" rev-parse --is-inside-work-tree >/dev/null 2>&1 || \
    die "could not find the Aurora Git worktree at $aurora_git_dir"

if ! source_binary="$(readlink -f -- "$1")"; then
    die "could not resolve showcase executable: $1"
fi
destination="$2"

[[ -f "$source_binary" && -x "$source_binary" ]] || \
    die "showcase executable is not an executable file: $source_binary"
[[ ! -e "$destination" && ! -L "$destination" ]] || \
    die "destination already exists: $destination"

source_commit="$(git -C "$source_git_dir" rev-parse HEAD)"
source_state="$(worktree_state "$source_git_dir")"
aurora_commit="$(git -C "$aurora_git_dir" rev-parse HEAD)"
aurora_state="$(worktree_state "$aurora_git_dir")"
packaged_at_utc="$(date -u +'%Y-%m-%dT%H:%M:%SZ')"

mkdir -p -- "$destination/bin"
install -m 0755 -- "$source_binary" "$destination/bin/smg-pc-showcase"
install -m 0755 -- "$script_dir/run-title-showcase.sh" \
    "$destination/run-title-showcase.sh"
install -m 0644 -- "$script_dir/README.md" "$destination/README.md"
install -m 0644 -- "$source_git_dir/LICENSE" "$destination/LICENSE"

checksum_line="$(sha256sum -- "$destination/bin/smg-pc-showcase")"
showcase_sha256="${checksum_line%% *}"
[[ "$showcase_sha256" =~ ^[[:xdigit:]]{64}$ ]] || \
    die "could not calculate the packaged executable SHA-256 digest"

build_info="$(<"$script_dir/BUILD-INFO.in")"
build_info="${build_info//@PACKAGED_AT_UTC@/$packaged_at_utc}"
build_info="${build_info//@SOURCE_GIT_COMMIT@/$source_commit}"
build_info="${build_info//@SOURCE_GIT_WORKTREE@/$source_state}"
build_info="${build_info//@AURORA_GIT_COMMIT@/$aurora_commit}"
build_info="${build_info//@AURORA_GIT_WORKTREE@/$aurora_state}"
build_info="${build_info//@SHOWCASE_SHA256@/$showcase_sha256}"
printf '%s\n' "$build_info" >"$destination/BUILD-INFO"

printf 'Created title-only showcase package at %s\n' "$destination"
printf 'Executable SHA-256: %s\n' "$showcase_sha256"
