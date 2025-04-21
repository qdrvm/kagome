# Image building for sanitizers
BUILD_TYPE_SANITIZER_IMAGES ?= RelWithDebInfo

# Sanitizer build directory reference
define sanitizer_build_dir
$(WORKING_DIR)/build_docker_$(ARCHITECTURE)_$(BUILD_TYPE_SANITIZER_IMAGES)_$(1)
endef

# Common function for building sanitizer images
define sanitizer_image_build
	echo "-- Building Docker image for $(1)..."; \
	$(MAKE) get_versions; \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	BUILD_TYPE_LOWER=$$(echo $(BUILD_TYPE_SANITIZER_IMAGES) | tr '[:upper:]' '[:lower:]'); \
	SANITIZER_PACKAGE_VERSION="$(KAGOME_DEB_PACKAGE_VERSION)-$(1)"; \
	SANITIZER_PACKAGE_NAME="kagome-$(1)"; \
	echo "Using package: $${SANITIZER_PACKAGE_NAME}=$${SANITIZER_PACKAGE_VERSION}"; \
	docker build --platform $(PLATFORM) \
		-t $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}_$(1):$${SHORT_COMMIT_HASH}-$(ARCHITECTURE) \
		-t $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}_$(1):$(SANITIZER_IMAGE_TAG)-$(ARCHITECTURE) \
		--secret id=google_creds,src=$(GOOGLE_APPLICATION_CREDENTIALS) \
		-f sanitizer_runner.Dockerfile \
		--build-arg BASE_IMAGE=$(OS_IMAGE_NAME) \
		--build-arg BASE_IMAGE_TAG=$(OS_IMAGE_TAG_WITH_HASH) \
		--build-arg ARCHITECTURE=$(ARCHITECTURE) \
		--build-arg KAGOME_PACKAGE_VERSION="$${SANITIZER_PACKAGE_VERSION}" \
		--build-arg PACKAGE_NAME="$${SANITIZER_PACKAGE_NAME}" \
		--build-arg PROJECT_ID=$(PROJECT_ID) \
		--target runner . && \
	echo "-- $(1) Docker image built successfully." || \
	{ echo "-- ERROR: $(1) Docker image build FAILED!"; exit 1; }
endef

# Common function for pushing sanitizer images
define sanitizer_image_push
	@echo "-- Pushing $(1) Docker image..."; \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	BUILD_TYPE_LOWER=$$(echo $(BUILD_TYPE_SANITIZER_IMAGES) | tr '[:upper:]' '[:lower:]'); \
	docker push $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}_$(1):$${SHORT_COMMIT_HASH}-$(ARCHITECTURE); \
	echo "KAGOME_$(shell echo $(1) | tr a-z A-Z)_IMAGE=$(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}_$(1):$${SHORT_COMMIT_HASH}-$(ARCHITECTURE)" | tee -a /tmp/docker_image_$(1).env; \
	docker push $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}_$(1):$(SANITIZER_IMAGE_TAG)-$(ARCHITECTURE); \
	echo "KAGOME_$(shell echo $(1) | tr a-z A-Z)_IMAGE_TAG=$(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}_$(1):$(SANITIZER_IMAGE_TAG)-$(ARCHITECTURE)" | tee -a /tmp/docker_image_$(1).env; \
	echo "-- $(1) Docker image pushed successfully."
endef

# Manifest creation for sanitizer images
define sanitizer_manifest_create
	@echo "-- Creating $(1) manifest..."; \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	BUILD_TYPE_LOWER=$$(echo $(BUILD_TYPE_SANITIZER_IMAGES) | tr '[:upper:]' '[:lower:]'); \
	docker manifest create $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}_$(1):$${SHORT_COMMIT_HASH} \
		--amend $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}_$(1):$${SHORT_COMMIT_HASH}-amd64 \
		--amend $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}_$(1):$${SHORT_COMMIT_HASH}-arm64; \
	docker manifest create $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}_$(1):$(SANITIZER_IMAGE_TAG) \
		--amend $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}_$(1):$(SANITIZER_IMAGE_TAG)-amd64 \
		--amend $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}_$(1):$(SANITIZER_IMAGE_TAG)-arm64; \
	echo "-- $(1) manifests created.";
endef

# Push sanitizer manifest
define sanitizer_manifest_push
	@echo "-- Pushing $(1) manifest..."; \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	BUILD_TYPE_LOWER=$$(echo $(BUILD_TYPE_SANITIZER_IMAGES) | tr '[:upper:]' '[:lower:]'); \
	docker manifest push $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}_$(1):$${SHORT_COMMIT_HASH}; \
	docker manifest push $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}_$(1):$(SANITIZER_IMAGE_TAG); \
	echo "-- $(1) manifests pushed.";
endef

# Manifest inspection for sanitizer images
define sanitizer_manifest_inspect
	@echo "-- Inspecting $(1) manifest..."; \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	BUILD_TYPE_LOWER=$$(echo $(BUILD_TYPE_SANITIZER_IMAGES) | tr '[:upper:]' '[:lower:]'); \
	docker manifest inspect $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}_$(1):$${SHORT_COMMIT_HASH} || echo "$(1) manifest not found!"; \
	docker manifest inspect $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}_$(1):$(SANITIZER_IMAGE_TAG) || echo "$(1) $(SANITIZER_IMAGE_TAG) manifest not found!"; \
	echo "-- $(1) manifests inspection completed.";
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

# Manifest create targets
kagome_sanitizer_manifest_create_asan:
	$(call sanitizer_manifest_create,asan)

kagome_sanitizer_manifest_create_tsan:
	$(call sanitizer_manifest_create,tsan)

kagome_sanitizer_manifest_create_ubsan:
	$(call sanitizer_manifest_create,ubsan)

kagome_sanitizer_manifest_create_asanubsan:
	$(call sanitizer_manifest_create,asanubsan)

# Manifest push targets
kagome_sanitizer_manifest_push_asan:
	$(call sanitizer_manifest_push,asan)

kagome_sanitizer_manifest_push_tsan:
	$(call sanitizer_manifest_push,tsan)

kagome_sanitizer_manifest_push_ubsan:
	$(call sanitizer_manifest_push,ubsan)

kagome_sanitizer_manifest_push_asanubsan:
	$(call sanitizer_manifest_push,asanubsan)

# Manifest creation and push combined targets
kagome_sanitizer_manifest_all_asan: kagome_sanitizer_manifest_create_asan kagome_sanitizer_manifest_push_asan
	@echo "-- ASAN manifest completed."

kagome_sanitizer_manifest_all_tsan: kagome_sanitizer_manifest_create_tsan kagome_sanitizer_manifest_push_tsan
	@echo "-- TSAN manifest completed."

kagome_sanitizer_manifest_all_ubsan: kagome_sanitizer_manifest_create_ubsan kagome_sanitizer_manifest_push_ubsan
	@echo "-- UBSAN manifest completed."

kagome_sanitizer_manifest_all_asanubsan: kagome_sanitizer_manifest_create_asanubsan kagome_sanitizer_manifest_push_asanubsan
	@echo "-- ASANUBSAN manifest completed."

# Inspection targets
kagome_sanitizer_manifest_inspect_asan:
	$(call sanitizer_manifest_inspect,asan)

kagome_sanitizer_manifest_inspect_tsan:
	$(call sanitizer_manifest_inspect,tsan)

kagome_sanitizer_manifest_inspect_ubsan:
	$(call sanitizer_manifest_inspect,ubsan)

kagome_sanitizer_manifest_inspect_asanubsan:
	$(call sanitizer_manifest_inspect,asanubsan)

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

# All sanitizers manifest targets
kagome_sanitizer_manifest_all: kagome_sanitizer_manifest_all_asan kagome_sanitizer_manifest_all_tsan kagome_sanitizer_manifest_all_ubsan kagome_sanitizer_manifest_all_asanubsan
	@echo "-- All sanitizer manifests completed."

kagome_sanitizer_manifest_inspect_all: kagome_sanitizer_manifest_inspect_asan kagome_sanitizer_manifest_inspect_tsan kagome_sanitizer_manifest_inspect_ubsan kagome_sanitizer_manifest_inspect_asanubsan
	@echo "-- All sanitizer manifests inspected."

.PHONY: kagome_image_build_asan kagome_image_build_tsan kagome_image_build_ubsan kagome_image_build_asanubsan \
        kagome_image_push_asan kagome_image_push_tsan kagome_image_push_ubsan kagome_image_push_asanubsan \
        kagome_sanitizer_manifest_create_asan kagome_sanitizer_manifest_create_tsan kagome_sanitizer_manifest_create_ubsan kagome_sanitizer_manifest_create_asanubsan \
        kagome_sanitizer_manifest_push_asan kagome_sanitizer_manifest_push_tsan kagome_sanitizer_manifest_push_ubsan kagome_sanitizer_manifest_push_asanubsan \
        kagome_sanitizer_manifest_all_asan kagome_sanitizer_manifest_all_tsan kagome_sanitizer_manifest_all_ubsan kagome_sanitizer_manifest_all_asanubsan \
        kagome_sanitizer_manifest_inspect_asan kagome_sanitizer_manifest_inspect_tsan kagome_sanitizer_manifest_inspect_ubsan kagome_sanitizer_manifest_inspect_asanubsan \
        kagome_image_all_asan kagome_image_all_tsan kagome_image_all_ubsan kagome_image_all_asanubsan \
        kagome_asan_all kagome_tsan_all kagome_ubsan_all kagome_asanubsan_all kagome_sanitizers_all \
        kagome_sanitizer_manifest_all kagome_sanitizer_manifest_inspect_all
