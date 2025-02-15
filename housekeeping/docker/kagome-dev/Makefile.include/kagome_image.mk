kagome_image_build:
	$(MAKE) get_versions; \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	BUILD_TYPE_LOWER=$$(echo $(BUILD_TYPE) | tr '[:upper:]' '[:lower:]'); \
	BUILD_TARGET=""; \
	if [ "$(BUILD_TYPE)" = "Debug" ] || [ "$(BUILD_TYPE)" = "RelWithDebInfo" ]; then \
		BUILD_TARGET="--target debug"; \
	fi; \
	docker build --platform $(PLATFORM) \
		-t $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH} \
		-t $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:latest \
		--secret id=google_creds,src=$(GOOGLE_APPLICATION_CREDENTIALS) \
		-f kagome_runner.Dockerfile \
		--build-arg BASE_IMAGE=$(OS_IMAGE_NAME) \
		--build-arg BASE_IMAGE_TAG=$(OS_IMAGE_TAG_WITH_HASH) \
		--build-arg ARCHITECTURE=$(ARCHITECTURE) \
		--build-arg KAGOME_PACKAGE_VERSION=$(KAGOME_PACKAGE_VERSION) \
		--build-arg PROJECT_ID=$(PROJECT_ID) \
		$${BUILD_TARGET} .

kagome_image_push:
	BUILD_TYPE_LOWER=$$(echo $(BUILD_TYPE) | tr '[:upper:]' '[:lower:]'); \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	docker push $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH} ; \
	echo "KAGOME_IMAGE=$(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH}" | tee /tmp/docker_image.env ; \
	docker push $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:latest ; \
	echo "KAGOME_IMAGE_LATEST=$(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:latest" | tee -a /tmp/docker_image.env ; \
	if [ "$(IS_MAIN_OR_TAG)" = "true" ]; then \
		if [ "$(GIT_REF_NAME)" = "master" ]; then \
			MAIN_TAG="$${SHORT_COMMIT_HASH}-master"; \
			docker tag $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH} \
			$(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${MAIN_TAG}; \
			docker push $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${MAIN_TAG}; \
			echo "KAGOME_IMAGE_MASTER=$(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${MAIN_TAG}" | tee -a /tmp/docker_image.env ; \
		elif [ -n "$(GIT_REF_NAME)" ]; then \
			docker tag $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH} \
			$(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${GIT_REF_NAME}; \
			docker push $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${GIT_REF_NAME}; \
			echo "KAGOME_IMAGE_TAG=$(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${GIT_REF_NAME}" | tee -a /tmp/docker_image.env ; \
		fi \
	fi

kagome_image_push_dockerhub:
	BUILD_TYPE_LOWER="release" ; \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	if [ "$(IS_MAIN_OR_TAG)" = "true" ]; then \
		if [ "$(GIT_REF_NAME)" = "master" ]; then \
			MAIN_TAG="$${SHORT_COMMIT_HASH}-master"; \
			docker tag $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH} \
			$(DOCKERHUB_REGISTRY_PATH):$${MAIN_TAG}; \
			docker tag $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH} \
			$(DOCKERHUB_REGISTRY_PATH):master; \
			docker push $(DOCKERHUB_REGISTRY_PATH):$${MAIN_TAG}; \
			echo "KAGOME_DOCKERHUB_IMAGE_MASTER=$(DOCKERHUB_REGISTRY_PATH):$${MAIN_TAG}" | tee /tmp/dockerhub_image.env ; \
			docker push $(DOCKERHUB_REGISTRY_PATH):master; \
			echo "KAGOME_DOCKERHUB_IMAGE_LATEST=$(DOCKERHUB_REGISTRY_PATH):master" | tee -a /tmp/dockerhub_image.env ; \
		elif [ -n "$(GIT_REF_NAME)" ]; then \
			docker tag $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH} \
			$(DOCKERHUB_REGISTRY_PATH):$${GIT_REF_NAME}; \
			docker tag $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH} \
			$(DOCKERHUB_REGISTRY_PATH):latest; \
			docker push $(DOCKERHUB_REGISTRY_PATH):$${GIT_REF_NAME}; \
			echo "KAGOME_DOCKERHUB_IMAGE_TAG=$(DOCKERHUB_REGISTRY_PATH):$${GIT_REF_NAME}" | tee /tmp/dockerhub_image.env ; \
			docker push $(DOCKERHUB_REGISTRY_PATH):latest; \
			echo "KAGOME_DOCKERHUB_IMAGE_LATEST=$(DOCKERHUB_REGISTRY_PATH):latest" | tee -a /tmp/dockerhub_image.env ; \
		fi \
	fi
