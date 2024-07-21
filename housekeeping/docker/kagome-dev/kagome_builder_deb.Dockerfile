ARG AUTHOR="k.azovtsev@qdrvm.io <Kirill Azovtsev>"

ARG BASE_IMAGE
ARG RUST_VERSION
ARG ARCHITECTURE=x86_64

FROM ${BASE_IMAGE}

ARG AUTHOR
ENV AUTHOR=${AUTHOR}
LABEL org.opencontainers.image.authors="${AUTHOR}"
LABEL org.opencontainers.image.description="Kagome builder image"

SHELL ["/bin/bash", "-c"]

RUN install_packages \
        build-essential \
        ccache \
        clang-format-16 \
        clang-tidy-16 \
        curl \
        dpkg-dev \
        g++-12 \
        gcc-12 \
        gdb \
        gdbserver \
        git \
        gpg \
        gpg-agent \
        lcov \
        libclang-rt-16-dev \
        libgmp10 \
        libnsl-dev \
        libseccomp-dev \
        llvm-16-dev \
        make \
        mold \
        nano \
        python3 \
        python3-pip \
        python3-venv \
        software-properties-common \
        unzip \
        vim \
        wget \
        zlib1g-dev

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
