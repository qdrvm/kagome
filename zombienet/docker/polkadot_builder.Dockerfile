ARG AUTHOR="k.azovtsev@qdrvm.io <Kirill Azovtsev>"

ARG ARCHITECTURE=x86_64
ARG SCCACHE_VERSION

ARG POLKADOT_SDK_RELEASE
ARG RUST_IMAGE

FROM ${RUST_IMAGE} AS polkadot-sdk-builder

LABEL org.opencontainers.image.authors="${AUTHOR}"
LABEL org.opencontainers.image.description="Polkadot SDK builder image"

WORKDIR /home/nonroot/
SHELL ["/bin/bash", "-c"]

ARG POLKADOT_SDK_RELEASE
ENV POLKADOT_SDK_RELEASE=$POLKADOT_SDK_RELEASE

RUN apt-get update && \
    apt-get install --no-install-recommends --yes \
            git \
            wget \
            openssl \
            ca-certificates \
            build-essential \
            clang \
            protobuf-compiler \
            libprotobuf-dev && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

RUN rustup default stable &&  \
    rustup target add wasm32-unknown-unknown &&  \
    rustup component add rust-src

RUN git clone --depth 1 --branch $POLKADOT_SDK_RELEASE https://github.com/paritytech/polkadot-sdk.git

ARG SCCACHE_VERSION
ENV SCCACHE_VERSION=$SCCACHE_VERSION
ARG ARCHITECTURE
ENV ARCHITECTURE=$ARCHITECTURE

# Version >0.7.4 has a bug - work with GCS is broken
RUN mkdir -p /tmp/download && \
    cd /tmp/download && \
    wget -O sccache.tar.gz https://github.com/mozilla/sccache/releases/download/v${SCCACHE_VERSION}/sccache-v${SCCACHE_VERSION}-${ARCHITECTURE}-unknown-linux-musl.tar.gz && \
    tar xzf sccache.tar.gz && \
    ls -la && \
    mv sccache-v*/sccache /usr/local/bin/sccache && \
    chmod +x /usr/local/bin/sccache ; \
    rm -rf /tmp/download

WORKDIR /home/nonroot/polkadot-sdk/
