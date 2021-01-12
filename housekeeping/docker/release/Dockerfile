FROM alpine:3

RUN apk add --no-cache libgcc libstdc++

WORKDIR /kagome
ENV PATH $PATH:/kagome

COPY kagome_full_syncing kagome_validating /usr/local/bin/
