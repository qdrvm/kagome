FROM alpine:3

RUN apk add --no-cache libgcc libstdc++

WORKDIR /kagome
ENV PATH $PATH:/kagome

COPY kagome_full_syncing kagome_validating kagome_block_producing /usr/local/bin/
