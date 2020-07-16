FROM alpine:3

RUN apk add --no-cache libgcc libstdc++

WORKDIR /kagome
ENV PATH $PATH:/kagome

COPY node/kagome_full_syncing/kagome_full_syncing node/kagome_validating/kagome_validating node/kagome_block_producing/kagome_block_producing ./
