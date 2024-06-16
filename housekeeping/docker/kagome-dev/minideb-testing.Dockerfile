# bookworm
ARG BASE_IMAGE=bitnami/minideb@sha256:6cc3baf349947d587a9cd4971e81ff3ffc0d17382f2b5b6de63d6542bff10c16
ARG RUST_VERSION=1.79.0

FROM ${BASE_IMAGE}

SHELL ["/bin/bash", "-c"]

ENV KAGOME_IN_DOCKER=1

RUN install_packages \
        gpg \
        gpg-agent \
        wget \
        vim \
        python3 \
        python3-pip \
        python3-venv \
        software-properties-common \
        gdb \
        gdbserver \
        curl

RUN install_packages \
        mold  \
        build-essential \
        gcc-12 \
        g++-12 \
        llvm-16-dev \
        clang-tidy-16 \
        clang-format-16 \
        libclang-rt-16-dev \
        make \
        git \
        ccache \
        lcov \
        zlib1g-dev \
        libgmp10 \
        libnsl-dev \
        libseccomp-dev \
        unzip

ARG RUST_VERSION
ENV RUST_VERSION=${RUST_VERSION}
ENV RUSTUP_HOME=/root/.rustup
ENV CARGO_HOME=/root/.cargo
ENV PATH="${CARGO_HOME}/bin:${PATH}"
RUN curl https://sh.rustup.rs -sSf | sh -s -- -y --default-toolchain ${RUST_VERSION} && \
    rustup default ${RUST_VERSION}

RUN python3 -m venv /venv
RUN /venv/bin/python3 -m pip install --no-cache-dir --upgrade pip
RUN /venv/bin/pip install --no-cache-dir cmake==3.25 scikit-build requests gitpython gcovr pyyaml

ENV HUNTER_PYTHON_LOCATION=/venv/bin/python3

ENV LLVM_ROOT=/usr/lib/llvm-16
ENV LLVM_DIR=/usr/lib/llvm-16/lib/cmake/llvm/
ENV PATH=${LLVM_ROOT}/bin:${LLVM_ROOT}/share/clang:${PATH}
ENV CC=gcc-12
ENV CXX=g++-12

RUN update-alternatives --install /usr/bin/python       python       /venv/bin/python3              90 && \
    update-alternatives --install /usr/bin/python       python       /usr/bin/python3               80 && \
    \
    update-alternatives --install /usr/bin/clang-tidy   clang-tidy   /usr/bin/clang-tidy-16         50 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-16       50 && \
    update-alternatives --install /usr/bin/clang        clang        /usr/lib/llvm-16/bin/clang-16  50 && \
    update-alternatives --install /usr/bin/clang++      clang++      /usr/bin/clang++-16            50 && \
    \
    update-alternatives --install /usr/bin/gcc          gcc          /usr/bin/gcc-12                90 && \
    update-alternatives --install /usr/bin/g++          g++          /usr/bin/g++-12                90 && \
    update-alternatives --install /usr/bin/gcov         gcov         /usr/bin/gcov-12               90

RUN install -m 0755 -d /etc/apt/keyrings && \
        curl -fsSL https://download.docker.com/linux/debian/gpg -o /etc/apt/keyrings/docker.asc && \
        chmod a+r /etc/apt/keyrings/docker.asc && \
        echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/debian \
          $(. /etc/os-release && echo "$VERSION_CODENAME") stable" | \
        tee /etc/apt/sources.list.d/docker.list > /dev/null && \
        apt-get update && \
        install_packages \
            docker-ce \
            docker-ce-cli \
            containerd.io
