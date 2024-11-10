ARG AUTHOR="k.azovtsev@qdrvm.io <Kirill Azovtsev>"

ARG BASE_IMAGE
ARG BASE_IMAGE_TAG
ARG RUST_VERSION
ARG ARCHITECTURE=x86_64

ARG OS_REL_VERSION=noble
ARG LLVM_VERSION=19
ARG GCC_VERSION=13

FROM ${BASE_IMAGE}:${BASE_IMAGE_TAG}

ARG AUTHOR
ENV AUTHOR=${AUTHOR}
LABEL org.opencontainers.image.authors="${AUTHOR}"
LABEL org.opencontainers.image.description="Kagome builder image"

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

ARG OS_REL_VERSION
ENV OS_REL_VERSION=${OS_REL_VERSION}
ARG LLVM_VERSION
ENV LLVM_VERSION=${LLVM_VERSION}
ARG GCC_VERSION
ENV GCC_VERSION=${GCC_VERSION}

COPY install_packages /usr/sbin/install_packages
RUN chmod 0755 /usr/sbin/install_packages

RUN install_packages \
        apt-transport-https \
        ca-certificates \
        gnupg \
        wget

RUN wget --progress=dot:giga -O - https://apt.llvm.org/llvm-snapshot.gpg.key | gpg --dearmor -o /usr/share/keyrings/llvm-archive-keyring.gpg
RUN echo "deb [signed-by=/usr/share/keyrings/llvm-archive-keyring.gpg] http://apt.llvm.org/${OS_REL_VERSION}/ llvm-toolchain-${OS_REL_VERSION}-${LLVM_VERSION} main" | \
        tee -a /etc/apt/sources.list.d/llvm.list

RUN install_packages \
        build-essential \
        ccache \
        clang-format-${LLVM_VERSION} \
        clang-tidy-${LLVM_VERSION} \
        libclang-rt-${LLVM_VERSION}-dev \
        llvm-${LLVM_VERSION}-dev \
        curl \
        dpkg-dev \
        g++-${GCC_VERSION} \
        gcc-${GCC_VERSION} \
        gdb \
        gdbserver \
        git \
        gpg \
        gpg-agent \
        lcov \
        libgmp10 \
        libnsl-dev \
        libseccomp-dev \
        make \
        mold \
        nano \
        python3 \
        python3-pip \
        python3-venv \
        software-properties-common \
        unzip \
        vim \
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

ENV LLVM_ROOT=/usr/lib/llvm-${LLVM_VERSION}
ENV LLVM_DIR=/usr/lib/llvm-${LLVM_VERSION}/lib/cmake/llvm/
ENV PATH=${LLVM_ROOT}/bin:${LLVM_ROOT}/share/clang:${PATH}
ENV CC=gcc-${GCC_VERSION}
ENV CXX=g++-${GCC_VERSION}

RUN update-alternatives --install /usr/bin/python       python       /venv/bin/python3              90 && \
    update-alternatives --install /usr/bin/python       python       /usr/bin/python3               80 && \
    \
    update-alternatives --install /usr/bin/clang-tidy   clang-tidy   /usr/bin/clang-tidy-${LLVM_VERSION}                      50 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-${LLVM_VERSION}                    50 && \
    update-alternatives --install /usr/bin/clang        clang        /usr/lib/llvm-${LLVM_VERSION}/bin/clang-${LLVM_VERSION}  50 && \
    update-alternatives --install /usr/bin/clang++      clang++      /usr/bin/clang++-${LLVM_VERSION}                         50 && \
    \
    update-alternatives --install /usr/bin/gcc          gcc          /usr/bin/gcc-${GCC_VERSION}    90 && \
    update-alternatives --install /usr/bin/g++          g++          /usr/bin/g++-${GCC_VERSION}    90 && \
    update-alternatives --install /usr/bin/gcov         gcov         /usr/bin/gcov-${GCC_VERSION}   90
