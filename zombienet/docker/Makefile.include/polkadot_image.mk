polkadot_builder:
	echo "-- Building polkadot_builder image for $(ARCHITECTURE) architecture..." ; \
	docker build --progress=plain --platform $(PLATFORM) \
		-t $(DOCKER_REGISTRY_PATH)polkadot_builder:$(CURRENT_DATE)-rust$(RUST_VERSION)-$(ARCHITECTURE) \
		-f polkadot_builder.Dockerfile \
		--build-arg USER_ID="5555" \
		--build-arg GROUP_ID="5555" \
		--build-arg USER_NAME=$(IN_DOCKER_USERNAME) \
		--build-arg RUST_VERSION=$(RUST_VERSION) \
		--build-arg SCCACHE_VERSION=$(SCCACHE_VERSION) \
		--build-arg BASE_IMAGE=$(OS_IMAGE_NAME) \
		--build-arg BASE_IMAGE_TAG=$(OS_IMAGE_TAG_WITH_HASH) .

polkadot_builder_push:
	echo "-- Pushing polkadot_builder image for $(ARCHITECTURE) architecture..." ; \
	docker push $(DOCKER_REGISTRY_PATH)polkadot_builder:$(CURRENT_DATE)-rust$(RUST_VERSION)-$(ARCHITECTURE)

polkadot_builder_push_manifest:
	echo "-- Creating and pushing manifest for polkadot_builder image..." ; \
	docker manifest create $(DOCKER_REGISTRY_PATH)polkadot_builder:$(CURRENT_DATE)-rust$(RUST_VERSION) \
		--amend $(DOCKER_REGISTRY_PATH)polkadot_builder:$(CURRENT_DATE)-rust$(RUST_VERSION)-amd64 \
		--amend $(DOCKER_REGISTRY_PATH)polkadot_builder:$(CURRENT_DATE)-rust$(RUST_VERSION)-arm64 && \
	docker manifest create $(DOCKER_REGISTRY_PATH)polkadot_builder:$(BUILDER_LATEST_TAG) \
		--amend $(DOCKER_REGISTRY_PATH)polkadot_builder:$(CURRENT_DATE)-rust$(RUST_VERSION)-amd64 \
		--amend $(DOCKER_REGISTRY_PATH)polkadot_builder:$(CURRENT_DATE)-rust$(RUST_VERSION)-arm64 && \
	docker manifest push $(DOCKER_REGISTRY_PATH)polkadot_builder:$(CURRENT_DATE)-rust$(RUST_VERSION) && \
	docker manifest push $(DOCKER_REGISTRY_PATH)polkadot_builder:$(BUILDER_LATEST_TAG)

polkadot_builder_all_arch:
	echo "-- Building and pushing polkadot_builder image for all architectures..." ; \
	$(MAKE) polkadot_builder PLATFORM=linux/amd64 ARCHITECTURE=amd64 && \
	$(MAKE) polkadot_builder_push PLATFORM=linux/amd64 ARCHITECTURE=amd64 && \
	$(MAKE) polkadot_builder PLATFORM=linux/arm64 ARCHITECTURE=arm64 && \
	$(MAKE) polkadot_builder_push PLATFORM=linux/arm64 ARCHITECTURE=arm64 && \
	$(MAKE) polkadot_builder_push_manifest

polkadot_builder_image_info:
	@echo "-- Polkadot Builder Image Info --"
	@echo POLKADOT_BUILDER_IMAGE:       $(DOCKER_REGISTRY_PATH)polkadot_builder:$(BUILDER_LATEST_TAG)
	@echo POLKADOT_BUILDER_AMD64_IMAGE: $(DOCKER_REGISTRY_PATH)polkadot_builder:$(CURRENT_DATE)-rust$(RUST_VERSION)-amd64
	@echo POLKADOT_BUILDER_ARM64_IMAGE: $(DOCKER_REGISTRY_PATH)polkadot_builder:$(CURRENT_DATE)-rust$(RUST_VERSION)-arm64

polkadot_builder_check_tag:
	@docker manifest inspect $(DOCKER_REGISTRY_PATH)polkadot_builder:$(CURRENT_DATE)-rust$(RUST_VERSION) > /dev/null 2>&1 && echo "true" || echo "false"

polkadot_builder_check_latest_tag:
	@docker manifest inspect $(DOCKER_REGISTRY_PATH)polkadot_builder:$(BUILDER_LATEST_TAG) > /dev/null 2>&1 && echo "true" || echo "false"
