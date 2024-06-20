FROM ${MINIDEB_IMAGE} as zombienet-bin

LABEL org.opencontainers.image.authors="${AUTHOR}"
LABEL org.opencontainers.image.description="Zombienet image"

ARG ZOMBIENET_RELEASE
ENV ZOMBIENET_RELEASE=$ZOMBIENET_RELEASE

RUN groupadd --gid 10000 nonroot && \
    useradd --home-dir /home/nonroot \
            --create-home \
            --shell /bin/bash \
            --gid nonroot \
            --groups nonroot \
            --uid 10000 nonroot

WORKDIR /home/nonroot/

RUN install_packages  \
        bash \
        wget  \
        nano \
        ca-certificates

SHELL ["/bin/bash", "-c"]

RUN mkdir -p /home/nonroot/bin
    
WORKDIR /home/nonroot/bin

ENV PATH="/home/nonroot/bin:${PATH}"

RUN wget https://github.com/paritytech/zombienet/releases/download/$ZOMBIENET_RELEASE/zombienet-linux-x64 && \
    chmod +x zombienet-linux-x64 && \
    ln -s /home/nonroot/bin/zombienet-linux-x64 /home/nonroot/bin/zombienet

# WARNING: Setup always returns 2, even when successful 
RUN zombienet setup -y polkadot polkadot-parachain; \
    EXIT_CODE=$?; \
    if [ $EXIT_CODE -eq 2 ]; then \
        echo "Command exited with code 2, continuing..."; \
    else \
        echo "Command exited with code $EXIT_CODE"; \
        exit $EXIT_CODE; \
    fi;

COPY --from=polkadot-sdk-builder /home/nonroot/polkadot-sdk/target/testnet/malus /home/nonroot/bin
COPY --from=polkadot-sdk-builder /home/nonroot/polkadot-sdk/target/testnet/adder-collator /home/nonroot/bin
COPY --from=polkadot-sdk-builder /home/nonroot/polkadot-sdk/target/testnet/undying-collator /home/nonroot/bin

RUN ./polkadot --version && \
    ./polkadot-parachain --version && \
    ./zombienet version  && \
    ./polkadot-execute-worker --version && \
    ./polkadot-prepare-worker --version && \
    ./malus --version && \
    ./adder-collator --version && \
    ./undying-collator --version


FROM zombienet-bin as final

LABEL org.opencontainers.image.authors="${AUTHOR}"
LABEL org.opencontainers.image.description="Zombienet Builder image"

ENV PATH="/home/nonroot/bin:${PATH}"
ARG ZOMBIENET_RELEASE
ENV ZOMBIENET_RELEASE=$ZOMBIENET_RELEASE
ARG POLKADOT_SDK_RELEASE
ENV POLKADOT_SDK_RELEASE=$POLKADOT_SDK_RELEASE

RUN install_packages  \
      curl  \
      gpg  \
      gpg-agent  \
      git \
      time \
      software-properties-common \
      libstdc++6  \
      libc6  \
      libnsl2 \
      libtinfo6 \
      libseccomp2 \
      libatomic1 \
      ssh
