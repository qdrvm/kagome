kagome_builder:
	docker build --progress=plain --platform $(PLATFORM) \
			--no-cache \
    		-t $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_IMAGE_TAG)-$(ARCHITECTURE) \
    		-t $(DOCKERHUB_BUILDER_PATH):$(BUILDER_IMAGE_TAG)-$(ARCHITECTURE) \
    		-f kagome_builder.Dockerfile \
    		--build-arg RUST_VERSION=$(RUST_VERSION) \
    		--build-arg GCC_VERSION=$(GCC_VERSION) \
    		--build-arg LLVM_VERSION=$(LLVM_VERSION) \
    		--build-arg BASE_IMAGE=$(OS_IMAGE_NAME) \
    		--build-arg BASE_IMAGE_TAG=$(OS_IMAGE_TAG_WITH_HASH) \
    		--build-arg OS_REL_VERSION=$(OS_REL_VERSION) \
    		--build-arg ARCHITECTURE=$(ARCHITECTURE) .

kagome_builder_all_arch:
	$(MAKE) kagome_builder PLATFORM=linux/amd64 ARCHITECTURE=amd64 && \
	$(MAKE) kagome_builder_push PLATFORM=linux/amd64 ARCHITECTURE=amd64 && \
	$(MAKE) kagome_builder PLATFORM=linux/arm64 ARCHITECTURE=arm64 && \
	$(MAKE) kagome_builder_push PLATFORM=linux/arm64 ARCHITECTURE=arm64 && \
	$(MAKE) kagome_builder_push_manifest

kagome_builder_push:
	docker push $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_IMAGE_TAG)-$(ARCHITECTURE)

kagome_builder_push_dockerhub:
	docker push $(DOCKERHUB_BUILDER_PATH):$(BUILDER_IMAGE_TAG)-$(ARCHITECTURE)

kagome_builder_push_manifest:
	docker manifest create $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_IMAGE_TAG) \
		--amend $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_IMAGE_TAG)-amd64 \
		--amend $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_IMAGE_TAG)-arm64 && \
	docker manifest create $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_LATEST_TAG) \
		--amend $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_IMAGE_TAG)-amd64 \
		--amend $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_IMAGE_TAG)-arm64 && \
	docker manifest push $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_IMAGE_TAG) && \
	docker manifest push $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_LATEST_TAG)

kagome_builder_push_dockerhub_manifest:
	docker manifest create $(DOCKERHUB_BUILDER_PATH):$(BUILDER_IMAGE_TAG) \
        --amend $(DOCKERHUB_BUILDER_PATH):$(BUILDER_IMAGE_TAG)-amd64 \
        --amend $(DOCKERHUB_BUILDER_PATH):$(BUILDER_IMAGE_TAG)-arm64 && \
    docker manifest create $(DOCKERHUB_BUILDER_PATH):$(BUILDER_LATEST_TAG) \
        --amend $(DOCKERHUB_BUILDER_PATH):$(BUILDER_IMAGE_TAG)-amd64 \
        --amend $(DOCKERHUB_BUILDER_PATH):$(BUILDER_IMAGE_TAG)-arm64 && \
    docker manifest push $(DOCKERHUB_BUILDER_PATH):$(BUILDER_IMAGE_TAG) && \
    docker manifest push $(DOCKERHUB_BUILDER_PATH):$(BUILDER_LATEST_TAG)

kagome_builder_check_tag:
	@docker manifest inspect $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_IMAGE_TAG) > /dev/null 2>&1 && echo "true" || echo "false"

kagome_builder_check_latest_tag:
	@docker manifest inspect $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_LATEST_TAG) > /dev/null 2>&1 && echo "true" || echo "false"

kagome_builder_image_info:
	@echo "-- Kagome Builder Image Info --"
	@echo "Registry Images:"
	@echo "  KAGOME_BUILDER_IMAGE:       $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_LATEST_TAG)"
	@echo "  KAGOME_BUILDER_IMAGE:       $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_IMAGE_TAG)"
	@echo "  KAGOME_BUILDER_AMD64_IMAGE: $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_IMAGE_TAG)-amd64"
	@echo "  KAGOME_BUILDER_ARM64_IMAGE: $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_IMAGE_TAG)-arm64"
	@echo "\n-- Docker Hub Images --"
	@echo "  DOCKERHUB_BUILDER_IMAGE:        $(DOCKERHUB_BUILDER_PATH):$(BUILDER_LATEST_TAG)"
	@echo "  DOCKERHUB_BUILDER_IMAGE:        $(DOCKERHUB_BUILDER_PATH):$(BUILDER_IMAGE_TAG)"
	@echo "  DOCKERHUB_BUILDER_AMD64_IMAGE:  $(DOCKERHUB_BUILDER_PATH):$(BUILDER_IMAGE_TAG)-amd64"
	@echo "  DOCKERHUB_BUILDER_ARM64_IMAGE:  $(DOCKERHUB_BUILDER_PATH):$(BUILDER_IMAGE_TAG)-arm64"
