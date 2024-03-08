ARG POLKADOT_SDK_RELEASE

FROM qdrvm/kagome:latest as kagome

FROM qdrvm/zombie-builder:$POLKADOT_SDK_RELEASE AS tester
COPY --from=kagome /usr/local/bin/kagome /home/nonroot/bin
RUN git clone https://github.com/qdrvm/kagome.git
RUN mkdir /home/nonroot/.local && \
   chown nonroot:nonroot /home/nonroot/.local && \
   chown nonroot:nonroot /tmp
USER nonroot
CMD zombienet-linux-x64 test -p native kagome/zombienet/0001-parachains-smoke-test/0001-parachains-smoke-test.zndsl && \
    zombienet-linux-x64 test -p native kagome/zombienet/0002-parachains-upgrade-smoke-tests/0002-parachains-upgrade-smoke-test.zndsl && \
    zombienet-linux-x64 test -p native kagome/zombienet/0003-parachains-smoke-test-cumulus/0003-parachains-smoke-test-cumulus.zndsl
