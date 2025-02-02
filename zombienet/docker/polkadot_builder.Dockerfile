# ===== Base definitions and initial installations as root =====
ARG AUTHOR="k.azovtsev@qdrvm.io <Kirill Azovtsev>"

ARG BASE_IMAGE
ARG BASE_IMAGE_TAG

ARG SCCACHE_VERSION
ARG POLKADOT_SDK_RELEASE
ARG RUST_VERSION

FROM ${BASE_IMAGE}:${BASE_IMAGE_TAG} AS polkadot-sdk-builder

# Set author and image labels
ARG AUTHOR
ENV AUTHOR=${AUTHOR}
LABEL org.opencontainers.image.authors="${AUTHOR}"
LABEL org.opencontainers.image.description="Polkadot SDK builder image"

# Use a temporary working directory for root operations
WORKDIR /root/
SHELL ["/bin/bash", "-xo", "pipefail", "-c"]

# Copy the install_packages script and set executable permissions
COPY install_packages /usr/sbin/install_packages
RUN chmod 0755 /usr/sbin/install_packages

# Set Polkadot SDK release environment variable and install required packages
ARG POLKADOT_SDK_RELEASE
ENV POLKADOT_SDK_RELEASE=$POLKADOT_SDK_RELEASE

RUN install_packages \
      git \
      wget \
      curl \
      openssl \
      ca-certificates \
      build-essential \
      clang \
      protobuf-compiler \
      libprotobuf-dev

# Create download directory and install sccache
ARG SCCACHE_VERSION
ENV SCCACHE_VERSION=$SCCACHE_VERSION
ENV DOWNLOAD_DIR=/tmp/download
RUN mkdir -p ${DOWNLOAD_DIR}

RUN ARCHITECTURE=$(uname -m); \
    if [ "$ARCHITECTURE" = "x86_64" ]; then \
        ARCHITECTURE="x86_64-unknown-linux-musl"; \
    elif [ "$ARCHITECTURE" = "aarch64" ]; then \
        ARCHITECTURE="aarch64-unknown-linux-musl"; \
    else \
        echo "-- Unsupported architecture: $ARCHITECTURE"; exit 1; \
    fi && \
    echo "-- Downloading sccache v${SCCACHE_VERSION} for ${ARCHITECTURE}" && \
    wget -q -O ${DOWNLOAD_DIR}/sccache.tar.gz https://github.com/mozilla/sccache/releases/download/v${SCCACHE_VERSION}/sccache-v${SCCACHE_VERSION}-${ARCHITECTURE}.tar.gz && \
    tar xzf ${DOWNLOAD_DIR}/sccache.tar.gz -C ${DOWNLOAD_DIR} && \
    mv ${DOWNLOAD_DIR}/sccache-v*/sccache /usr/local/bin/sccache && \
    chmod +x /usr/local/bin/sccache ; \
    rm -rf ${DOWNLOAD_DIR} ; \
    sccache --show-stats
# Version >0.7.4 has a bug - work with GCS is broken

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

USER ${USER_NAME}
WORKDIR /home/${USER_NAME}

# Install Rust toolchain
ARG RUST_VERSION
ENV RUST_VERSION=${RUST_VERSION}
ENV RUSTUP_HOME=/home/${USER_NAME}/.rustup
ENV CARGO_HOME=/home/${USER_NAME}/.cargo
ENV PATH="${CARGO_HOME}/bin:${PATH}"
RUN curl https://sh.rustup.rs -sSf | sh -s -- -y --default-toolchain ${RUST_VERSION} && \
    rustup default ${RUST_VERSION} && \
    rustup target add wasm32-unknown-unknown && \
    rustup component add rust-src
