ARG POLKADOT_RELEASE_GLOBAL
ARG POLKADOT_RELEASE_GLOBAL_NUMERIC
ARG POLKADOT_SDK_RELEASE
ARG ZOMBIENET_RELEASE

# bookworm
ARG BASE_IMAGE=bitnami/minideb@sha256:6cc3baf349947d587a9cd4971e81ff3ffc0d17382f2b5b6de63d6542bff10c16
ARG RUST_IMAGE=rust:1.79-slim-bookworm

ARG AUTHOR="k.azovtsev@qdrvm.io <Kirill Azovtsev>"

FROM ${RUST_IMAGE} AS polkadot-sdk-builder

LABEL org.opencontainers.image.authors="${AUTHOR}"
LABEL org.opencontainers.image.description="Polkadot SDK builder image"

WORKDIR /home/nonroot/

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
            libprotobuf-dev && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

RUN rustup default stable &&  \
    rustup target add wasm32-unknown-unknown &&  \
    rustup component add rust-src

RUN git clone --depth 1 --branch $POLKADOT_SDK_RELEASE https://github.com/paritytech/polkadot-sdk.git

WORKDIR /home/nonroot/polkadot-sdk/
