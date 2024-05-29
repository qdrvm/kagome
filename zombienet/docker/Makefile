BRANCH_NAME ?= master
KAGOME_IMAGE ?= qdrvm/kagome:latest
ZOMBIE_TESTER_IMAGE ?= qdrvm/zombie-tester:latest

builder_last:
	python version.py https://github.com/paritytech/polkadot-sdk && \
	python version.py https://github.com/paritytech/zombienet && \
	make fetch_and_run

fetch_and_run:
	if [ -f polkadot-sdk-versions.txt ]; then \
		POLKADOT_SDK_RELEASE=$$(grep 'polkadot_format_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
		POLKADOT_RELEASE_GLOBAL=$$(grep 'short_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
		POLKADOT_RELEASE_GLOBAL_NUMERIC=$$(grep 'numeric_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
		ZOMBIENET_RELEASE=$$(grep 'short_version:' zombienet-versions.txt | cut -d ' ' -f 2); \
		DOCKER_BUILDKIT=0 docker build -t qdrvm/zombie-builder:$$POLKADOT_SDK_RELEASE -t qdrvm/zombie-builder:latest \
		-f builder.Dockerfile \
		--build-arg ZOMBIENET_RELEASE=$$ZOMBIENET_RELEASE \
		--build-arg POLKADOT_SDK_RELEASE=$$POLKADOT_SDK_RELEASE \
		--build-arg POLKADOT_RELEASE_GLOBAL=$$POLKADOT_RELEASE_GLOBAL \
		--build-arg POLKADOT_RELEASE_GLOBAL_NUMERIC=$$POLKADOT_RELEASE_GLOBAL_NUMERIC . ; \
	else \
		echo "One or more files are missing."; \
	fi

builder_push:
	if [ -f polkadot-sdk-versions.txt ]; then \
  		POLKADOT_SDK_RELEASE=$$(grep 'polkadot_format_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
		docker push qdrvm/zombie-builder:$$POLKADOT_SDK_RELEASE ; \
		docker push qdrvm/zombie-builder:latest ; \
	else \
		echo "One or more files are missing."; \
	fi	

builder:
	DOCKER_BUILDKIT=0 docker build -t qdrvm/zombie-builder:latest -f builder.Dockerfile --build-arg CUMULUS_RELEASE=v0.9.420 --build-arg POLKADOT_RELEASE=v0.9.42 --build-arg POLKADOT_RELEASE_GLOBAL=v0.9.42 .

tester:
	python3 version.py https://github.com/paritytech/polkadot-sdk && \
	if [ -f polkadot-sdk-versions.txt ]; then \
		POLKADOT_SDK_RELEASE=$$(grep 'polkadot_format_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
		BRANCH_NAME="$(BRANCH_NAME)" \
		KAGOME_IMAGE="$(KAGOME_IMAGE)" \
		DOCKER_BUILDKIT=0 docker build --platform linux/amd64 --no-cache -t qdrvm/zombie-tester:latest -t zombie-tester:latest -f tester.Dockerfile \
		--build-arg BRANCH_NAME=$$BRANCH_NAME \
		--build-arg KAGOME_IMAGE=$$KAGOME_IMAGE \
		--build-arg POLKADOT_SDK_RELEASE=$$POLKADOT_SDK_RELEASE . ; \
	else \
		echo "One or more files are missing."; \
	fi

tests:
	docker run --platform linux/amd64 -it qdrvm/zombie-tester:latest bash

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

test-polkadot-functional-0001-parachains-pvf:
	ZOMBIE_TESTER_IMAGE=$(ZOMBIE_TESTER_IMAGE)
	docker run --platform linux/amd64 $$ZOMBIE_TESTER_IMAGE zombienet-linux-x64 test -p native kagome/zombienet/polkadot/functional/0001-parachains-pvf.zndsl

test-polkadot-functional-0002-parachains-disputes:
	ZOMBIE_TESTER_IMAGE=$(ZOMBIE_TESTER_IMAGE)
	docker run $$ZOMBIE_TESTER_IMAGE zombienet-linux-x64 test -p native kagome/zombienet/polkadot/functional/0002-parachains-disputes.zndsl

test-polkadot-functional-0003-beefy-and-mmr:
	ZOMBIE_TESTER_IMAGE=$(ZOMBIE_TESTER_IMAGE)
	docker run $$ZOMBIE_TESTER_IMAGE zombienet-linux-x64 test -p native kagome/zombienet/polkadot/functional/0003-beefy-and-mmr.zndsl

test-polkadot-functional-0004-parachains-garbage-candidate:
	ZOMBIE_TESTER_IMAGE=$(ZOMBIE_TESTER_IMAGE)
	docker run $$ZOMBIE_TESTER_IMAGE zombienet-linux-x64 test -p native kagome/zombienet/polkadot/functional/0004-parachains-garbage-candidate.zndsl

test-polkadot-functional-0005-parachains-disputes-past-session:
	ZOMBIE_TESTER_IMAGE=$(ZOMBIE_TESTER_IMAGE)
	docker run $$ZOMBIE_TESTER_IMAGE zombienet-linux-x64 test -p native kagome/zombienet/polkadot/functional/0005-parachains-disputes-past-session.zndsl

test-polkadot-functional-0006-parachains-max-tranche0:
	ZOMBIE_TESTER_IMAGE=$(ZOMBIE_TESTER_IMAGE)
	docker run $$ZOMBIE_TESTER_IMAGE zombienet-linux-x64 test -p native kagome/zombienet/polkadot/functional/0006-parachains-max-tranche0.zndsl

test-polkadot-functional-0007-dispute-freshly-finalized:
	ZOMBIE_TESTER_IMAGE=$(ZOMBIE_TESTER_IMAGE)
	docker run $$ZOMBIE_TESTER_IMAGE zombienet-linux-x64 test -p native kagome/zombienet/polkadot/functional/0007-dispute-freshly-finalized.zndsl

test-polkadot-functional-0008-dispute-old-finalized:
	ZOMBIE_TESTER_IMAGE=$(ZOMBIE_TESTER_IMAGE)
	docker run $$ZOMBIE_TESTER_IMAGE zombienet-linux-x64 test -p native kagome/zombienet/polkadot/functional/0008-dispute-old-finalized.zndsl

test-polkadot-functional-0009-approval-voting-coalescing:
	ZOMBIE_TESTER_IMAGE=$(ZOMBIE_TESTER_IMAGE)
	docker run $$ZOMBIE_TESTER_IMAGE zombienet-linux-x64 test -p native kagome/zombienet/polkadot/functional/0009-approval-voting-coalescing.zndsl

test-polkadot-functional-0010-validator-disabling:
	ZOMBIE_TESTER_IMAGE=$(ZOMBIE_TESTER_IMAGE)
	docker run $$ZOMBIE_TESTER_IMAGE zombienet-linux-x64 test -p native kagome/zombienet/polkadot/functional/0010-validator-disabling.zndsl

test-polkadot-functional-0011-async-backing-6-seconds-rate:
	ZOMBIE_TESTER_IMAGE=$(ZOMBIE_TESTER_IMAGE)
	docker run $$ZOMBIE_TESTER_IMAGE zombienet-linux-x64 test -p native kagome/zombienet/polkadot/functional/0011-async-backing-6-seconds-rate.zndsl
