ARG AUTHOR="k.azovtsev@qdrvm.io <Kirill Azovtsev>"

ARG BASE_IMAGE
ARG BASE_IMAGE_TAG

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

RUN install_packages \
      git \
      wget \
      curl \
      openssl \
      ca-certificates \
      build-essential \
      clang \
      protobuf-compiler \
      libprotobuf-dev \
      sccache

ARG RUST_VERSION
ENV RUST_VERSION=${RUST_VERSION}
ENV RUSTUP_HOME=/root/.rustup
ENV CARGO_HOME=/root/.cargo
ENV PATH="${CARGO_HOME}/bin:${PATH}"
RUN curl https://sh.rustup.rs -sSf | sh -s -- -y --default-toolchain ${RUST_VERSION}

RUN rustup default ${RUST_VERSION} &&  \
    rustup target add wasm32-unknown-unknown &&  \
    rustup component add rust-src
