# Image building for sanitizers
BUILD_TYPE_SANITIZER_IMAGES ?= RelWithDebInfo

# Sanitizer build directory reference
define sanitizer_build_dir
$(WORKING_DIR)/build_docker_$(ARCHITECTURE)_$(BUILD_TYPE_SANITIZER_IMAGES)_$(1)
endef

# Common function for building sanitizer images
define sanitizer_image_build
	@echo "-- Building Docker image for $(1)..."; \
	$(MAKE) get_versions; \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	BUILD_TYPE_LOWER=$$(echo $(BUILD_TYPE_SANITIZER_IMAGES) | tr '[:upper:]' '[:lower:]'); \
	docker build --platform $(PLATFORM) \
		-t $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}_$(1):$${SHORT_COMMIT_HASH} \
		-t $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}_$(1):latest \
		--secret id=google_creds,src=$(GOOGLE_APPLICATION_CREDENTIALS) \
		-f kagome_runner.Dockerfile \
		--build-arg BASE_IMAGE=$(OS_IMAGE_NAME) \
		--build-arg BASE_IMAGE_TAG=$(OS_IMAGE_TAG_WITH_HASH) \
		--build-arg ARCHITECTURE=$(ARCHITECTURE) \
		--build-arg KAGOME_PACKAGE_VERSION=$(KAGOME_DEB_PACKAGE_VERSION)-$(1) \
		--build-arg PACKAGE_NAME=kagome-$(1) \
		--build-arg PROJECT_ID=$(PROJECT_ID) \
		--target debug .; \
	echo "-- $(1) Docker image built successfully."
endef

# Common function for pushing sanitizer images
define sanitizer_image_push
	@echo "-- Pushing $(1) Docker image..."; \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	BUILD_TYPE_LOWER=$$(echo $(BUILD_TYPE_SANITIZER_IMAGES) | tr '[:upper:]' '[:lower:]'); \
	docker push $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}_$(1):$${SHORT_COMMIT_HASH}; \
	echo "KAGOME_$(shell echo $(1) | tr a-z A-Z)_IMAGE=$(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}_$(1):$${SHORT_COMMIT_HASH}" | tee -a /tmp/docker_image_$(1).env; \
	docker push $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}_$(1):latest; \
	echo "KAGOME_$(shell echo $(1) | tr a-z A-Z)_IMAGE_LATEST=$(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}_$(1):latest" | tee -a /tmp/docker_image_$(1).env; \
	echo "-- $(1) Docker image pushed successfully."
endef

# Image build targets
kagome_image_build_asan:
	$(call sanitizer_image_build,asan)

kagome_image_build_tsan:
	$(call sanitizer_image_build,tsan)

kagome_image_build_ubsan:
	$(call sanitizer_image_build,ubsan)

kagome_image_build_asanubsan:
	$(call sanitizer_image_build,asanubsan)

# Image push targets
kagome_image_push_asan:
	$(call sanitizer_image_push,asan)

kagome_image_push_tsan:
	$(call sanitizer_image_push,tsan)

kagome_image_push_ubsan:
	$(call sanitizer_image_push,ubsan)

kagome_image_push_asanubsan:
	$(call sanitizer_image_push,asanubsan)

# Build and push convenience targets
kagome_image_all_asan: kagome_image_build_asan kagome_image_push_asan
	@echo "-- ASAN Docker image build and push completed."

kagome_image_all_tsan: kagome_image_build_tsan kagome_image_push_tsan
	@echo "-- TSAN Docker image build and push completed."

kagome_image_all_ubsan: kagome_image_build_ubsan kagome_image_push_ubsan
	@echo "-- UBSAN Docker image build and push completed."

kagome_image_all_asanubsan: kagome_image_build_asanubsan kagome_image_push_asanubsan
	@echo "-- ASAN+UBSAN Docker image build and push completed."

# Complete workflow targets - build package and image
kagome_asan_all: kagome_docker_build_asan kagome_upload_asan_package kagome_image_all_asan
	@echo "-- Complete ASAN build workflow completed."

kagome_tsan_all: kagome_docker_build_tsan kagome_upload_tsan_package kagome_image_all_tsan
	@echo "-- Complete TSAN build workflow completed."

kagome_ubsan_all: kagome_docker_build_ubsan kagome_upload_ubsan_package kagome_image_all_ubsan
	@echo "-- Complete UBSAN build workflow completed."

kagome_asanubsan_all: kagome_docker_build_asanubsan kagome_upload_asanubsan_package kagome_image_all_asanubsan
	@echo "-- Complete ASAN+UBSAN build workflow completed."

# Build all sanitizers
kagome_sanitizers_all: kagome_asan_all kagome_tsan_all kagome_ubsan_all kagome_asanubsan_all
	@echo "-- All sanitizer builds completed."
