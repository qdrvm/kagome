#Image with kagome
FROM soramitsu/kagome:latest as kagome

FROM soramitsu/zombie-builder:latest AS tester
COPY --from=kagome /usr/local/bin/kagome /home/nonroot/bin
RUN git clone https://github.com/soramitsu/kagome.git
RUN cp /home/nonroot/bin/wasm_binary_spec_version_incremented.rs.compact.compressed.wasm \
        /home/nonroot/kagome/zombienet/0004-runtime-upgrade/ && \
    cp /home/nonroot/bin/wasm_binary_spec_version_incremented.rs.compact.compressed.wasm \
        /home/nonroot/kagome/zombienet/0004-runtime-upgrade-kagome/
RUN mkdir /home/nonroot/.local && \
   chown nonroot:nonroot /home/nonroot/.local && \
   chown nonroot:nonroot /tmp
USER nonroot
CMD zombienet-linux-x64 test -p native kagome/zombienet/0001-parachains-smoke-test/0001-parachains-smoke-test.zndsl && \
    zombienet-linux-x64 test -p native kagome/zombienet/0002-parachains-upgrade-smoke-tests/0002-parachains-upgrade-smoke-test.zndsl && \
    zombienet-linux-x64 test -p native kagome/zombienet/0003-parachains-smoke-test-cumulus/0003-parachains-smoke-test-cumulus.zndsl
