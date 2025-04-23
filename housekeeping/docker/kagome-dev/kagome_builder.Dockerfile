# ===== Base definitions and initial installations as root =====
ARG AUTHOR="k.azovtsev@qdrvm.io <Kirill Azovtsev>"

ARG BASE_IMAGE
ARG BASE_IMAGE_TAG

ARG OS_REL_VERSION

ARG RUST_VERSION
ARG LLVM_VERSION
ARG GCC_VERSION

FROM ${BASE_IMAGE}:${BASE_IMAGE_TAG} AS kagome-builder

# Set author and image labels
ARG AUTHOR
ENV AUTHOR=${AUTHOR}
LABEL org.opencontainers.image.authors="${AUTHOR}"
LABEL org.opencontainers.image.description="Kagome builder image"

# Use a temporary working directory for root operations
WORKDIR /root/
SHELL ["/bin/bash", "-xo", "pipefail", "-c"]

# Copy the install_packages script and set executable permissions
COPY install_packages /usr/sbin/install_packages
RUN chmod 0755 /usr/sbin/install_packages

# Set OS release version environment variable and install required packages
ARG OS_REL_VERSION
ENV OS_REL_VERSION=${OS_REL_VERSION}
ARG LLVM_VERSION
ENV LLVM_VERSION=${LLVM_VERSION}
ARG GCC_VERSION
ENV GCC_VERSION=${GCC_VERSION}

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
        ninja-build \
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
        zlib1g-dev \
        gosu

# Create non-root user and switch to it
ARG USER_ID=1000
ARG GROUP_ID=1000
ARG USER_NAME=builder
ENV USER_ID=${USER_ID}
ENV GROUP_ID=${GROUP_ID}
ENV USER_NAME=${USER_NAME}

# Create group with the specified GID, and user with the specified UID. Switch to the user.
RUN groupadd -g ${GROUP_ID} ${USER_NAME} && \
    useradd -m -d /home/${USER_NAME} -u ${USER_ID} -g ${USER_NAME} ${USER_NAME}

WORKDIR /home/${USER_NAME}

# Install Rust and Python toolchains
ARG RUST_VERSION
ENV RUST_VERSION=${RUST_VERSION}
ENV RUSTUP_HOME=/home/${USER_NAME}/.rustup
ENV CARGO_HOME=/home/${USER_NAME}/.cargo
ENV PATH="${CARGO_HOME}/bin:${PATH}"
RUN curl https://sh.rustup.rs -sSf | sh -s -- -y --default-toolchain ${RUST_VERSION} && \
    rustup default ${RUST_VERSION}

WORKDIR /home/${USER_NAME}/workspace

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

COPY entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod 0755 /usr/local/bin/entrypoint.sh

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]

HEALTHCHECK --interval=5s --timeout=2s --start-period=0s --retries=100 \
  CMD test -f /tmp/entrypoint_done || exit 1
