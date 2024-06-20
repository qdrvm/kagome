ARG AUTHOR="k.azovtsev@qdrvm.io <Kirill Azovtsev>"

ARG POLKADOT_RELEASE_GLOBAL
ARG POLKADOT_RELEASE_GLOBAL_NUMERIC
ARG POLKADOT_SDK_RELEASE
ARG ZOMBIENET_RELEASE

ARG POLKADOT_BUILDER_IMAGE
ARG MINIDEB_IMAGE




FROM ${POLKADOT_BUILDER_IMAGE} AS polkadot-sdk-builder

LABEL org.opencontainers.image.authors="${AUTHOR}"
LABEL org.opencontainers.image.description="Polkadot SDK binary builder image"

RUN cargo update  \
      -p test-parachain-adder-collator \
      -p polkadot-test-malus \
      -p test-parachain-undying-collator

RUN cargo build --profile testnet  \
      -p test-parachain-adder-collator \
      -p polkadot-test-malus \
      -p test-parachain-undying-collator

RUN echo "Listing files in /home/nonroot/polkadot-sdk/target: " && \
    find /home/nonroot/polkadot-sdk/target/ -maxdepth 2 -print




