ARG POLKADOT_SDK_RELEASE

FROM qdrvm/kagome:latest as kagome

FROM qdrvm/zombie-builder:$POLKADOT_SDK_RELEASE AS tester
COPY --from=kagome /usr/local/bin/kagome /home/nonroot/bin
RUN git clone https://github.com/qdrvm/kagome.git -b test/polkadot-functional-zombietests
RUN mkdir /home/nonroot/.local && \
   chown nonroot:nonroot /home/nonroot/.local && \
   chown nonroot:nonroot /tmp
USER nonroot
#CMD zombienet-linux-x64 test -p native kagome/zombienet/0001-parachains-smoke-test/0001-parachains-smoke-test.zndsl && \
#    zombienet-linux-x64 test -p native kagome/zombienet/0002-parachains-upgrade-smoke-tests/0002-parachains-upgrade-smoke-test.zndsl && \
#    zombienet-linux-x64 test -p native kagome/zombienet/0003-parachains-smoke-test-cumulus/0003-parachains-smoke-test-cumulus.zndsl


# Prepare runtimes
RUN mkdir /tmp/chain-specs
RUN polkadot build-spec --chain rococo-local --raw > /tmp/chain-specs/rococo-local.raw
RUN polkadot build-spec --chain westend-local --raw > /tmp/chain-specs/westend-local.raw
# TODO(kamilsa) #2099: replace kagome launch with timeout below with precompile command
RUN timeout 10m kagome --chain /tmp/chain-specs/rococo-local.raw --tmp --validator --wasm-execution Compiled || echo "kagome command timed out"
RUN timeout 10m kagome --chain /tmp/chain-specs/westend-local.raw --tmp --validator --wasm-execution Compiled || echo "kagome command timed out"
