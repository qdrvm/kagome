ARG AUTHOR="k.azovtsev@qdrvm.io <Kirill Azovtsev>"

ARG BASE_IMAGE
ARG BASE_IMAGE_TAG
ARG ARCHITECTURE=x86_64

ARG SCCACHE_VERSION
ARG POLKADOT_SDK_RELEASE
ARG RUST_VERSION

FROM ${BASE_IMAGE}:${BASE_IMAGE_TAG} AS polkadot-sdk-builder

ARG AUTHOR
ENV AUTHOR=${AUTHOR}
LABEL org.opencontainers.image.authors="${AUTHOR}"
LABEL org.opencontainers.image.description="Polkadot SDK builder image"

WORKDIR /home/nonroot/
SHELL ["/bin/bash", "-o", "pipefail", "-c"]

COPY install_packages /usr/sbin/install_packages
RUN chmod 0755 /usr/sbin/install_packages

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

ARG RUST_VERSION
ENV RUST_VERSION=${RUST_VERSION}
ENV RUSTUP_HOME=/root/.rustup
ENV CARGO_HOME=/root/.cargo
ENV PATH="${CARGO_HOME}/bin:${PATH}"
RUN curl https://sh.rustup.rs -sSf | sh -s -- -y --default-toolchain ${RUST_VERSION}

RUN rustup default ${RUST_VERSION} &&  \
    rustup target add wasm32-unknown-unknown &&  \
    rustup component add rust-src

RUN git clone --depth 1 --branch $POLKADOT_SDK_RELEASE https://github.com/paritytech/polkadot-sdk.git

ARG SCCACHE_VERSION
ENV SCCACHE_VERSION=$SCCACHE_VERSION
ARG ARCHITECTURE
ENV ARCHITECTURE=$ARCHITECTURE

RUN mkdir -p /tmp/download
    
WORKDIR /tmp/download

# Version >0.7.4 has a bug - work with GCS is broken
RUN wget -q -O sccache.tar.gz https://github.com/mozilla/sccache/releases/download/v${SCCACHE_VERSION}/sccache-v${SCCACHE_VERSION}-${ARCHITECTURE}-unknown-linux-musl.tar.gz && \
    tar xzf sccache.tar.gz && \
    ls -la && \
    mv sccache-v*/sccache /usr/local/bin/sccache && \
    chmod +x /usr/local/bin/sccache ; \
    rm -rf /tmp/download

WORKDIR /home/nonroot/polkadot-sdk/
