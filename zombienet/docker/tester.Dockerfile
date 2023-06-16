#Image with kagome
FROM soramitsu/kagome:latest as kagome

FROM soramitsu/zombie-builder:latest AS tester
COPY --from=kagome /usr/local/bin/kagome /home/nonroot/bin
RUN git clone https://github.com/soramitsu/kagome.git
USER nonroot
CMD zombienet-linux-x64 test -p native kagome/zombienet/0001-parachains-smoke-test/0001-parachains-smoke-test.zndsl kagome/zombienet/0002-parachains-upgrade-smoke-tests/0002-parachains-upgrade-smoke-test.zndsl kagome/zombienet/0003-parachains-smoke-test-cumulus/0003-parachains-smoke-test-cumulus.zndsl
