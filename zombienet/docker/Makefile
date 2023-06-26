builder_last:
	pip install GitPython==3.1.31 && \
	python version.py https://github.com/paritytech/polkadot.git && \
	python version.py https://github.com/paritytech/cumulus.git && \
	export $(shell [ ! -n "polkadot-version.txt" ] || cat polkadot-version.txt | grep -v \
    --perl-regexp '^('$$(env | sed 's/=.*//'g | tr '\n' '|')')\=') && \
	export $(shell [ ! -n "cumulus-version.txt" ] || cat cumulus-version.txt | grep -v \
    --perl-regexp '^('$$(env | sed 's/=.*//'g | tr '\n' '|')')\=') && \
	DOCKER_BUILDKIT=0 docker build -t soramitsu/zombie-builder:latest -f builder.Dockerfile --build-arg CUMULUS_RELEASE=$$CUMULUS_RELEASE --build-arg POLKADOT_RELEASE=$$POLKADOT_RELEASE --build-arg POLKADOT_RELEASE_GLOBAL=$$POLKADOT_RELEASE .
#rm -rf polkadot &&
#rm -rf cumulus

builder_push:
	docker push soramitsu/zombie-builder:latest

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