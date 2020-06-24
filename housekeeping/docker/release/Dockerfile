FROM ubuntu:18.04

RUN apt-get update && \
    apt-get install --no-install-recommends -y \
        libasan5 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /kagome
ENV PATH $PATH:/kagome

COPY node/kagome_full_syncing/kagome_full_syncing node/kagome_validating/kagome_validating node/kagome_block_producing/kagome_block_producing ./
