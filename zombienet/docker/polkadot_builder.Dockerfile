ARG AUTHOR="k.azovtsev@qdrvm.io <Kirill Azovtsev>"

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
            openssl \
            ca-certificates \
            build-essential \
            clang \
            protobuf-compiler \
            libprotobuf-dev \
            sccache && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

RUN rustup default stable &&  \
    rustup target add wasm32-unknown-unknown &&  \
    rustup component add rust-src

RUN git clone --depth 1 --branch $POLKADOT_SDK_RELEASE https://github.com/paritytech/polkadot-sdk.git

WORKDIR /home/nonroot/polkadot-sdk/
