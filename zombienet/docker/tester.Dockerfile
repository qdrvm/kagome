ARG POLKADOT_SDK_RELEASE
ARG HASH_COMMIT

FROM qdrvm/kagome:latest as kagome

FROM qdrvm/zombie-builder:$POLKADOT_SDK_RELEASE AS tester
COPY --from=kagome /usr/local/bin/kagome /home/nonroot/bin
RUN git clone https://github.com/qdrvm/kagome.git && \
    cd kagome && \
    git checkout $HASH_COMMIT \
    cd ..

# TODO(kamilsa): #2099 Remove this once we have a proper way to precompile
RUN mkdir /tmp/kagome/
RUN wget --no-check-certificate 'https://drive.usercontent.google.com/uc?id=1HlZ581MfFH9xtIXCj20NyUuDAC5Ip4CN&export=download&confirm=yes' -O /tmp/runtime_cache.zip
RUN apt update && apt install -y unzip
RUN unzip /tmp/runtime_cache.zip -d /
RUN rm /tmp/runtime_cache.zip
RUN chown -R nonroot:nonroot /tmp/kagome/

RUN mkdir /home/nonroot/.local && \
   chown nonroot:nonroot /home/nonroot/.local && \
   chown nonroot:nonroot /tmp
USER nonroot
