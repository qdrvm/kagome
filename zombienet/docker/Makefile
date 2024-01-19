builder_last:
	python version.py https://github.com/paritytech/polkadot-sdk && \
	make fetch_and_run

fetch_and_run:
	if [ -f polkadot-sdk-short-version.txt ] && [ -f polkadot-sdk-polkadot-version.txt ]; then \
  		POLKADOT_SDK_RELEASE=$(shell cat polkadot-sdk-polkadot-version.txt); \
		POLKADOT_RELEASE_GLOBAL=$(shell cat polkadot-sdk-short-version.txt); \
		DOCKER_BUILDKIT=0 docker build -t qdrvm/zombie-builder:$$POLKADOT_SDK_RELEASE -f builder.Dockerfile \
		--build-arg POLKADOT_SDK_RELEASE=$$POLKADOT_SDK_RELEASE \
		--build-arg POLKADOT_RELEASE_GLOBAL=$$POLKADOT_RELEASE_GLOBAL . ; \
	else \
		echo "One or more files are missing."; \
	fi

builder_push:
	docker push qdrvm/zombie-builder:latest

builder:
	DOCKER_BUILDKIT=0 docker build -t qdrvm/zombie-builder:latest -f builder.Dockerfile --build-arg CUMULUS_RELEASE=v0.9.420 --build-arg POLKADOT_RELEASE=v0.9.42 --build-arg POLKADOT_RELEASE_GLOBAL=v0.9.42 .

tester:
	docker build --no-cache -t qdrvm/zombie-tester:latest -f tester.Dockerfile .

tests:
	docker run -it qdrvm/zombie-tester:latest

test0001:
	docker run qdrvm/zombie-tester:latest zombienet-linux-x64 test -p native kagome/zombienet/0001-parachains-smoke-test/0001-parachains-smoke-test.zndsl

test0001_kagome:
	docker run qdrvm/zombie-tester:latest zombienet-linux-x64 test -p native kagome/zombienet/0001-parachains-smoke-test-kagome/0001-parachains-smoke-test-kagome.zndsl

test0002:
	docker run qdrvm/zombie-tester:latest zombienet-linux-x64 test -p native kagome/zombienet/0002-parachains-upgrade-smoke-tests/0002-parachains-upgrade-smoke-test.zndsl

test0002_kagome:
	docker run qdrvm/zombie-tester:latest zombienet-linux-x64 test -p native kagome/zombienet/0002-parachains-upgrade-smoke-tests-kagome/0002-parachains-upgrade-smoke-test-kagome.zndsl

test0003:
	docker run qdrvm/zombie-tester:latest zombienet-linux-x64 test -p native kagome/zombienet/0003-parachains-smoke-test-cumulus/0003-parachains-smoke-test-cumulus.zndsl

test0003_kagome:
	docker run qdrvm/zombie-tester:latest zombienet-linux-x64 test -p native kagome/zombienet/0003-parachains-smoke-test-cumulus-kagome/0003-parachains-smoke-test-cumulus-kagome.zndsl

test0004:
	docker run qdrvm/zombie-tester:latest zombienet-linux-x64 test -p native kagome/zombienet/0004-runtime-upgrade/0004-runtime-upgrade.zndsl

test0004_kagome:
	docker run qdrvm/zombie-tester:latest zombienet-linux-x64 test -p native kagome/zombienet/0004-runtime-upgrade-kagome/0004-runtime-upgrade-kagome.zndsl

test0005:
	docker run qdrvm/zombie-tester:latest zombienet-linux-x64 test -p native kagome/zombienet/0005-dispute-valid-block-1-kagome/0005-dispute-valid-block-1-kagome.zndsl

test0006:
	docker run qdrvm/zombie-tester:latest zombienet-linux-x64 test -p native kagome/zombienet/0006-parachains-disputes-1-kagome/0006-parachains-disputes-1-kagome.zndsl

test0008:
	docker run qdrvm/zombie-tester:latest zombienet-linux-x64 test -p native kagome/zombienet/0008-deregister-register-validator-smoke/0008-deregister-register-validator-smoke.zndsl

test0009:
	docker run qdrvm/zombie-tester:latest zombienet-linux-x64 test -p native kagome/zombienet/0009-basic-warp-sync/0009-basic-warp-sync.zndsl

test0010:
	docker run qdrvm/zombie-tester:latest zombienet-linux-x64 test -p native kagome/zombienet/0010-validators-warp-sync/0010-validators-warp-sync.zndsl

test0011:
	docker run qdrvm/zombie-tester:latest zombienet-linux-x64 test -p native kagome/zombienet/0011-block-building-warp-sync/0011-block-building-warp-sync.zndsl
