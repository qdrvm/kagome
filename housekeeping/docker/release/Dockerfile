FROM ubuntu:18.04

RUN apt-get update && apt-get upgrade -y && \
    apt-get install --no-install-recommends -y \
        gcc-8 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /kagome

COPY node/kagome_full_syncing/kagome_full_syncing node/kagome_validating/kagome_validating ./
