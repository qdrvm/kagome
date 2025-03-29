kagome_image_build:
	$(MAKE) get_versions; \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	BUILD_TYPE_LOWER=$$(echo $(BUILD_TYPE) | tr '[:upper:]' '[:lower:]'); \
	BUILD_TARGET=""; \
    if [ "$(BUILD_TYPE)" = "Debug" ] || [ "$(BUILD_TYPE)" = "RelWithDebInfo" ]; then \
        BUILD_TARGET="--target debug"; \
    else \
        BUILD_TARGET="--target release"; \
    fi; \
	docker build --platform $(PLATFORM) \
		-t $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH}-$(ARCHITECTURE) \
		--secret id=google_creds,src=$(GOOGLE_APPLICATION_CREDENTIALS) \
		-f kagome_runner.Dockerfile \
		--build-arg BASE_IMAGE=$(OS_IMAGE_NAME) \
		--build-arg BASE_IMAGE_TAG=$(OS_IMAGE_TAG_WITH_HASH) \
		--build-arg ARCHITECTURE=$(ARCHITECTURE) \
		--build-arg KAGOME_DEB_PACKAGE_VERSION=$(KAGOME_DEB_PACKAGE_VERSION) \
		--build-arg PROJECT_ID=$(PROJECT_ID) \
		$${BUILD_TARGET} .

kagome_image_build_all_arch:
	$(MAKE) kagome_image_build PLATFORM=linux/amd64 ARCHITECTURE=amd64 && \
	$(MAKE) kagome_image_push PLATFORM=linux/amd64 ARCHITECTURE=amd64 && \
	$(MAKE) kagome_image_build PLATFORM=linux/arm64 ARCHITECTURE=arm64 && \
	$(MAKE) kagome_image_push PLATFORM=linux/arm64 ARCHITECTURE=arm64 && \
	$(MAKE) kagome_image_push_manifest

kagome_image_push:
	BUILD_TYPE_LOWER=$$(echo $(BUILD_TYPE) | tr '[:upper:]' '[:lower:]'); \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	docker push $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH}-$(ARCHITECTURE) ; \
	echo "KAGOME_IMAGE_$(ARCHITECTURE)=$(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH}-$(ARCHITECTURE)" ; \
	if [ "$(IS_MAIN_OR_TAG)" = "true" ]; then \
		if [ "$(GIT_REF_NAME)" = "master" ]; then \
			MAIN_TAG="$${SHORT_COMMIT_HASH}-master"; \
			docker tag $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH}-$(ARCHITECTURE) \
			$(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${MAIN_TAG}-$(ARCHITECTURE); \
			docker push $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${MAIN_TAG}-$(ARCHITECTURE); \
			echo "KAGOME_IMAGE_MASTER_$(ARCHITECTURE)=$(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${MAIN_TAG}-$(ARCHITECTURE)" ; \
		elif [ -n "$(GIT_REF_NAME)" ]; then \
			docker tag $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH}-$(ARCHITECTURE) \
			$(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${GIT_REF_NAME}-$(ARCHITECTURE); \
			docker push $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${GIT_REF_NAME}-$(ARCHITECTURE); \
			echo "KAGOME_IMAGE_TAG_$(ARCHITECTURE)=$(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${GIT_REF_NAME}-$(ARCHITECTURE)" ; \
		fi \
	fi

kagome_image_push_manifest:
	BUILD_TYPE_LOWER=$$(echo $(BUILD_TYPE) | tr '[:upper:]' '[:lower:]'); \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	docker manifest create $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH} \
		--amend $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH}-amd64 \
		--amend $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH}-arm64 && \
	docker manifest create $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:latest \
		--amend $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH}-amd64 \
		--amend $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH}-arm64 && \
	docker manifest push $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH} && \
	docker manifest push $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:latest && \
	if [ "$(IS_MAIN_OR_TAG)" = "true" ]; then \
		if [ "$(GIT_REF_NAME)" = "master" ]; then \
			MAIN_TAG="$${SHORT_COMMIT_HASH}-master"; \
			docker manifest create $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${MAIN_TAG} \
				--amend $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${MAIN_TAG}-amd64 \
				--amend $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${MAIN_TAG}-arm64 && \
			docker manifest push $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${MAIN_TAG}; \
			echo "KAGOME_IMAGE_MASTER=$(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${MAIN_TAG}" ; \
		elif [ -n "$(GIT_REF_NAME)" ]; then \
			docker manifest create $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${GIT_REF_NAME} \
				--amend $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${GIT_REF_NAME}-amd64 \
				--amend $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${GIT_REF_NAME}-arm64 && \
			docker manifest push $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${GIT_REF_NAME}; \
			echo "KAGOME_IMAGE_TAG=$(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${GIT_REF_NAME}" ; \
		fi \
	fi

kagome_image_push_dockerhub:
	BUILD_TYPE_LOWER="release" ; \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	if [ "$(IS_MAIN_OR_TAG)" = "true" ]; then \
		if [ "$(GIT_REF_NAME)" = "master" ]; then \
			MAIN_TAG="$${SHORT_COMMIT_HASH}-master"; \
			docker tag $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH}-$(ARCHITECTURE) \
			$(DOCKERHUB_REGISTRY_PATH):$${MAIN_TAG}-$(ARCHITECTURE); \
			docker push $(DOCKERHUB_REGISTRY_PATH):$${MAIN_TAG}-$(ARCHITECTURE); \
			echo "KAGOME_DOCKERHUB_IMAGE_MASTER_$(ARCHITECTURE)=$(DOCKERHUB_REGISTRY_PATH):$${MAIN_TAG}-$(ARCHITECTURE)" ; \
		elif [ -n "$(GIT_REF_NAME)" ]; then \
			docker tag $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH}-$(ARCHITECTURE) \
			$(DOCKERHUB_REGISTRY_PATH):$${GIT_REF_NAME}-$(ARCHITECTURE); \
			docker push $(DOCKERHUB_REGISTRY_PATH):$${GIT_REF_NAME}-$(ARCHITECTURE); \
			echo "KAGOME_DOCKERHUB_IMAGE_TAG_$(ARCHITECTURE)=$(DOCKERHUB_REGISTRY_PATH):$${GIT_REF_NAME}-$(ARCHITECTURE)" ; \
		fi \
	fi

kagome_image_push_dockerhub_manifest:
	BUILD_TYPE_LOWER="release" ; \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	if [ "$(IS_MAIN_OR_TAG)" = "true" ]; then \
		if [ "$(GIT_REF_NAME)" = "master" ]; then \
			MAIN_TAG="$${SHORT_COMMIT_HASH}-master"; \
			docker manifest create $(DOCKERHUB_REGISTRY_PATH):$${MAIN_TAG} \
				--amend $(DOCKERHUB_REGISTRY_PATH):$${MAIN_TAG}-amd64 \
				--amend $(DOCKERHUB_REGISTRY_PATH):$${MAIN_TAG}-arm64 && \
			docker manifest create $(DOCKERHUB_REGISTRY_PATH):master \
				--amend $(DOCKERHUB_REGISTRY_PATH):$${MAIN_TAG}-amd64 \
				--amend $(DOCKERHUB_REGISTRY_PATH):$${MAIN_TAG}-arm64 && \
			docker manifest push $(DOCKERHUB_REGISTRY_PATH):$${MAIN_TAG} && \
			docker manifest push $(DOCKERHUB_REGISTRY_PATH):master; \
			echo "KAGOME_DOCKERHUB_IMAGE_MASTER=$(DOCKERHUB_REGISTRY_PATH):$${MAIN_TAG}" ; \
			echo "KAGOME_DOCKERHUB_IMAGE_LATEST=$(DOCKERHUB_REGISTRY_PATH):master" ; \
		elif [ -n "$(GIT_REF_NAME)" ]; then \
			docker manifest create $(DOCKERHUB_REGISTRY_PATH):$${GIT_REF_NAME} \
				--amend $(DOCKERHUB_REGISTRY_PATH):$${GIT_REF_NAME}-amd64 \
				--amend $(DOCKERHUB_REGISTRY_PATH):$${GIT_REF_NAME}-arm64 && \
			docker manifest create $(DOCKERHUB_REGISTRY_PATH):latest \
				--amend $(DOCKERHUB_REGISTRY_PATH):$${GIT_REF_NAME}-amd64 \
				--amend $(DOCKERHUB_REGISTRY_PATH):$${GIT_REF_NAME}-arm64 && \
			docker manifest push $(DOCKERHUB_REGISTRY_PATH):$${GIT_REF_NAME} && \
			docker manifest push $(DOCKERHUB_REGISTRY_PATH):latest; \
			echo "KAGOME_DOCKERHUB_IMAGE_TAG=$(DOCKERHUB_REGISTRY_PATH):$${GIT_REF_NAME}" ; \
			echo "KAGOME_DOCKERHUB_IMAGE_LATEST=$(DOCKERHUB_REGISTRY_PATH):latest" ; \
		fi \
	fi

kagome_image_build_all_arch_dockerhub: kagome_image_build_all_arch
	$(MAKE) kagome_image_push_dockerhub PLATFORM=linux/amd64 ARCHITECTURE=amd64 && \
	$(MAKE) kagome_image_push_dockerhub PLATFORM=linux/arm64 ARCHITECTURE=arm64 && \
	$(MAKE) kagome_image_push_dockerhub_manifest

kagome_image_info:
	@echo "-- Kagome Image Info --"
	@BUILD_TYPE_LOWER=$$(echo $(BUILD_TYPE) | tr '[:upper:]' '[:lower:]'); \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2 2>/dev/null || echo "unknown"); \
	echo "Registry Images:"; \
	echo "  KAGOME_IMAGE:       $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:latest"; \
	echo "  KAGOME_IMAGE:       $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH}"; \
	echo "  KAGOME_AMD64_IMAGE: $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH}-amd64"; \
	echo "  KAGOME_ARM64_IMAGE: $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH}-arm64"; \
	echo "\n-- Docker Hub Images --"; \
	echo "  DOCKERHUB_IMAGE:        $(DOCKERHUB_REGISTRY_PATH):latest"; \
	echo "  DOCKERHUB_IMAGE:        $(DOCKERHUB_REGISTRY_PATH):$${SHORT_COMMIT_HASH}"; \
	echo "  DOCKERHUB_AMD64_IMAGE:  $(DOCKERHUB_REGISTRY_PATH):$${SHORT_COMMIT_HASH}-amd64"; \
	echo "  DOCKERHUB_ARM64_IMAGE:  $(DOCKERHUB_REGISTRY_PATH):$${SHORT_COMMIT_HASH}-arm64"

.PHONY: kagome_image_build kagome_image_build_all_arch kagome_image_push kagome_image_push_manifest \
        kagome_image_push_dockerhub kagome_image_push_dockerhub_manifest kagome_image_info
