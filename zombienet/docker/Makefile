builder:
	DOCKER_BUILDKIT=0 docker build -t soramitsu/zombie-builder:latest -f builder.Dockerfile --build-arg CUMULUS_RELEASE=v0.9.420 --build-arg POLKADOT_RELEASE=v0.9.42 --build-arg POLKADOT_RELEASE_GLOBAL=v0.9.42 .
#	DOCKER_BUILDKIT=0 docker build -t soramitsu/zombie-builder -f builder.Dockerfile --build-arg CUMULUS_RELEASE=v0.9.420 --build-arg POLKADOT_RELEASE=v0.9.42 .
#   git clone https://github.com/paritytech/cumulus.git && cd cumulus
# 	cd cumulus && \
# 	git ls-remote --tags --refs --sort tag origin "refs/tags/v[0-9]*\.[0-9]*\.[0-9]*" | grep -v -e "-rc*" | cut -d'/' -f3 | tail -n 1 && \
# 	export CUMULUS_RELEASE="${git ls-remote --tags --refs --sort tag origin "refs/tags/v[0-9]*\.[0-9]*\.[0-9]*" | grep -v -e "-rc*" | cut -d'/' -f3 | tail -n 1}"

tester:
	docker build -t soramitsu/zombie-tester:latest -f tester.Dockerfile .

tests:
	docker run -it soramitsu/zombie-tester:latest

test0001:
	docker run soramitsu/zombie-tester:latest zombienet-linux-x64 test -p native kagome/zombienet/0001-parachains-smoke-test/0001-parachains-smoke-test.zndsl

test0002:
	docker run soramitsu/zombie-tester:latest zombienet-linux-x64 test -p native kagome/zombienet/0002-parachains-upgrade-smoke-tests/0002-parachains-upgrade-smoke-test.zndsl

test0003:
	docker run soramitsu/zombie-tester:latest zombienet-linux-x64 test -p native kagome/zombienet/0003-parachains-smoke-test-cumulus/0003-parachains-smoke-test-cumulus.zndsl