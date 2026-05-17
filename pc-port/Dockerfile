# syntax=docker/dockerfile:1

FROM docker.io/ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive
ARG LLVM_VERSION=22
ARG XMAKE_VERSION=3.0.8
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=1000

RUN apt-get update && apt-get install -y --no-install-recommends \
        bash \
        build-essential \
        ca-certificates \
        cmake \
        curl \
        file \
        git \
        gnupg \
        lsb-release \
        make \
        mesa-utils \
        mesa-vulkan-drivers \
        ninja-build \
        p7zip-full \
        pkg-config \
        python3 \
        software-properties-common \
        sudo \
        vulkan-tools \
        vulkan-validationlayers \
        wget \
        x11-apps \
        xauth \
        xz-utils \
        zlib1g-dev \
        zsh \
        libegl1-mesa-dev \
        libgl1-mesa-dev \
        libglu1-mesa-dev \
        libreadline-dev \
        libvulkan-dev \
        libx11-dev \
        libx11-xcb-dev \
        libxcursor-dev \
        libxext-dev \
        libxfixes-dev \
        libxi-dev \
        libxinerama-dev \
        libxkbcommon-dev \
        libxrandr-dev \
        libxrender-dev \
    && rm -rf /var/lib/apt/lists/*

RUN wget -qO /tmp/llvm.sh https://apt.llvm.org/llvm.sh \
    && chmod +x /tmp/llvm.sh \
    && /tmp/llvm.sh "${LLVM_VERSION}" all \
    && rm /tmp/llvm.sh \
    && for tool in clang clang++ clangd clang-format clang-tidy lld lldb llvm-ar llvm-config llvm-ranlib llvm-strip; do \
        if [ -e "/usr/bin/${tool}-${LLVM_VERSION}" ]; then \
            ln -sf "/usr/bin/${tool}-${LLVM_VERSION}" "/usr/local/bin/${tool}"; \
        fi; \
    done \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN build_dir="$(mktemp -d)" \
    && case "${XMAKE_VERSION}" in \
        v*) xmake_tag="${XMAKE_VERSION}" ;; \
        *) xmake_tag="v${XMAKE_VERSION}" ;; \
    esac \
    && git clone --depth=1 --branch "${xmake_tag}" --recurse-submodules --shallow-submodules https://github.com/xmake-io/xmake.git "${build_dir}" \
    && if [ -f "${build_dir}/configure" ]; then \
        (cd "${build_dir}" && ./configure); \
    fi \
    && make -C "${build_dir}" --no-print-directory -j"$(nproc)" \
    && make -C "${build_dir}" --no-print-directory install PREFIX=/usr/local \
    && rm -rf "${build_dir}" \
    && printf '%s\n' '#!/usr/bin/env sh' 'exec /usr/local/bin/xmake lua private.xrepo "$@"' > /usr/local/bin/xrepo \
    && chmod 755 /usr/local/bin/xrepo \
    && XMAKE_ROOT=y xmake --version \
    && XMAKE_ROOT=y xrepo --version

COPY container/orig/ /workspaces/pcport/orig/

RUN if getent group "${USER_GID}" >/dev/null; then \
        user_group="$(getent group "${USER_GID}" | cut -d: -f1)"; \
    else \
        groupadd --gid "${USER_GID}" "${USERNAME}"; \
        user_group="${USERNAME}"; \
    fi \
    && if getent passwd "${USERNAME}" >/dev/null; then \
        usermod --uid "${USER_UID}" --gid "${user_group}" --home "/home/${USERNAME}" --shell /bin/zsh "${USERNAME}"; \
    elif existing_user="$(getent passwd "${USER_UID}" | cut -d: -f1)" && [ -n "${existing_user}" ]; then \
        usermod --login "${USERNAME}" --gid "${user_group}" --home "/home/${USERNAME}" --move-home --shell /bin/zsh "${existing_user}"; \
    else \
        useradd --uid "${USER_UID}" --gid "${user_group}" -m -s /bin/zsh "${USERNAME}"; \
    fi \
    && echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" > "/etc/sudoers.d/${USERNAME}" \
    && chmod 0440 "/etc/sudoers.d/${USERNAME}" \
    && install -d -m 0700 -o "${USERNAME}" -g "${user_group}" /tmp/xdg-runtime \
    && install -d -m 0755 -o "${USERNAME}" -g "${user_group}" /workspaces/pcport/pc-port \
    && chown -R "${USERNAME}:${user_group}" /workspaces/pcport

ENV CC=clang \
    CXX=clang++ \
    CMAKE_GENERATOR=Ninja \
    XDG_RUNTIME_DIR=/tmp/xdg-runtime \
    XMAKE_COLORTERM=nocolor

USER ${USERNAME}
WORKDIR /workspaces/pcport/pc-port

CMD ["/bin/zsh"]
