ARG POLKADOT_SDK_RELEASE
ARG BRANCH_NAME
ARG KAGOME_IMAGE

FROM $KAGOME_IMAGE as kagome

FROM qdrvm/zombie-builder:$POLKADOT_SDK_RELEASE-testing AS tester
ARG BRANCH_NAME
ENV BRANCH_NAME=${BRANCH_NAME}

COPY --from=kagome /usr/local/bin/kagome /home/nonroot/bin

WORKDIR /home/nonroot

RUN echo "Cloning branch: ${BRANCH_NAME}"
RUN git clone https://github.com/qdrvm/kagome.git -b ${BRANCH_NAME}

RUN mkdir -p /tmp/kagome
RUN cd kagome/zombienet && ./precompile.sh

RUN mkdir /home/nonroot/.local && \
    chown nonroot:nonroot /home/nonroot/.local && \
    chown -R nonroot:nonroot /tmp
USER nonroot
