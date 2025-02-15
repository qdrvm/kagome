zombie_tester:
	docker build --progress=plain --platform $(PLATFORM) \
		--no-cache \
		-t $(DOCKER_REGISTRY_PATH)zombie_tester:$(POLKADOT_SDK_RELEASE)_$(ZOMBIENET_RELEASE)-$(ARCHITECTURE) \
		--secret id=google_creds,src=$(GOOGLE_APPLICATION_CREDENTIALS) \
		-f zombie_tester.Dockerfile \
		--build-arg USER_ID=$(USER_ID) \
		--build-arg GROUP_ID=$(GROUP_ID) \
		--build-arg USER_NAME=$(IN_DOCKER_USERNAME) \
		--build-arg BASE_IMAGE=$(OS_IMAGE_NAME) \
		--build-arg BASE_IMAGE_TAG=$(OS_IMAGE_TAG_WITH_HASH) \
		--build-arg PROJECT_ID=$(PROJECT_ID) \
		--build-arg POLKADOT_DEB_PACKAGE_VERSION="$(POLKADOT_DEB_PACKAGE_VERSION)" \
		--build-arg ZOMBIENET_RELEASE=$(ZOMBIENET_RELEASE) .

zombie_tester_push:
	docker push $(DOCKER_REGISTRY_PATH)zombie_tester:$(POLKADOT_SDK_RELEASE)_$(ZOMBIENET_RELEASE)-$(ARCHITECTURE)

zombie_tester_push_manifest:
	docker manifest create $(DOCKER_REGISTRY_PATH)zombie_tester:$(POLKADOT_SDK_RELEASE)_$(ZOMBIENET_RELEASE) \
		--amend $(DOCKER_REGISTRY_PATH)zombie_tester:$(POLKADOT_SDK_RELEASE)_$(ZOMBIENET_RELEASE)-amd64 \
		--amend $(DOCKER_REGISTRY_PATH)zombie_tester:$(POLKADOT_SDK_RELEASE)_$(ZOMBIENET_RELEASE)-arm64 && \
	docker manifest create $(DOCKER_REGISTRY_PATH)zombie_tester:$(TESTER_LATEST_TAG) \
		--amend $(DOCKER_REGISTRY_PATH)zombie_tester:$(POLKADOT_SDK_RELEASE)_$(ZOMBIENET_RELEASE)-amd64 \
		--amend $(DOCKER_REGISTRY_PATH)zombie_tester:$(POLKADOT_SDK_RELEASE)_$(ZOMBIENET_RELEASE)-arm64 && \
	docker manifest push $(DOCKER_REGISTRY_PATH)zombie_tester:$(POLKADOT_SDK_RELEASE)_$(ZOMBIENET_RELEASE) && \
	docker manifest push $(DOCKER_REGISTRY_PATH)zombie_tester:$(TESTER_LATEST_TAG)

zombie_tester_all_arch:
	$(MAKE) zombie_tester PLATFORM=linux/amd64 ARCHITECTURE=amd64 && \
	$(MAKE) zombie_tester_push PLATFORM=linux/amd64 ARCHITECTURE=amd64 && \
	$(MAKE) zombie_tester PLATFORM=linux/arm64 ARCHITECTURE=arm64 && \
	$(MAKE) zombie_tester_push PLATFORM=linux/arm64 ARCHITECTURE=arm64 && \
	$(MAKE) zombie_tester_push_manifest

zombie_tester_image_info:
	@echo "---------------------------------"
	@echo ZOMBIE_TESTER_IMAGE:       $(DOCKER_REGISTRY_PATH)zombie_tester:$(TESTER_LATEST_TAG)
	@echo ZOMBIE_TESTER_AMD64_IMAGE: $(DOCKER_REGISTRY_PATH)zombie_tester:$(POLKADOT_SDK_RELEASE)_$(ZOMBIENET_RELEASE)-amd64
	@echo ZOMBIE_TESTER_ARM64_IMAGE: $(DOCKER_REGISTRY_PATH)zombie_tester:$(POLKADOT_SDK_RELEASE)_$(ZOMBIENET_RELEASE)-arm64

zombie_tester_check_tag:
	@docker manifest inspect $(DOCKER_REGISTRY_PATH)zombie_tester:$(POLKADOT_SDK_RELEASE)_$(ZOMBIENET_RELEASE) > /dev/null 2>&1 && echo "true" || echo "false"
