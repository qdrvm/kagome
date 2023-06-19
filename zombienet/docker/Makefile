builder:
	DOCKER_BUILDKIT=0 docker build -t soramitsu/zombie-builder:latest -f builder.Dockerfile --build-arg CUMULUS_RELEASE=v0.9.420 --build-arg POLKADOT_RELEASE=v0.9.42 --build-arg POLKADOT_RELEASE_GLOBAL=v0.9.42 .

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