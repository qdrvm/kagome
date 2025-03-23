# Sanitizer cache paths
define sanitizer_cache_dir
$(CACHE_PATH)/$(ARCHITECTURE)_$(1)
endef

# Cache upload allowed check
CACHE_UPLOAD_ALLOWED ?= false
CAN_UPLOAD_CACHE := $(shell if [ "$(CACHE_UPLOAD_ALLOWED)" = "true" ] || [ "$(GIT_REF_NAME)" = "master" ] || $(IS_TAG); then echo true; else echo false; fi)

# Cache files for sanitizers
define sanitizer_cache_file
$(call sanitizer_cache_dir,$(1))/cache_$(ARCHITECTURE)_$(BUILD_TYPE_SANITIZERS)_$(1).tar.zst
endef

# Generate cache for a specific sanitizer
define generate_sanitizer_cache
	@echo "-- Generating $(1) cache archive..."
	mkdir -p $(call sanitizer_cache_dir,$(1))
	cd $(CACHE_PATH) && tar -cf - \
		--use-compress-program="zstd -19 -T$(COMPRESSION_THREADS)" \
		$(ARCHITECTURE)_$(1)/.cargo \
		$(ARCHITECTURE)_$(1)/.hunter \
		$(ARCHITECTURE)_$(1)/.cache > $(call sanitizer_cache_file,$(1))
	@echo "-- $(1) cache generated at $(call sanitizer_cache_file,$(1))"
endef

# Extract cache for a specific sanitizer
define extract_sanitizer_cache
	@echo "-- Extracting $(1) cache archive..."
	if [ -f $(call sanitizer_cache_file,$(1)) ]; then \
		mkdir -p $(call sanitizer_cache_dir,$(1)); \
		cd $(CACHE_PATH) && tar -xf $(call sanitizer_cache_file,$(1)) -v --use-compress-program="zstd -d"; \
		echo "-- $(1) cache extracted."; \
		true; \
	else \
		echo "-- $(1) cache file not found."; \
		false; \
	fi
endef

# Get cache for a specific sanitizer
define get_sanitizer_cache
	@echo "-- Getting $(1) cache..."
	if [ "$(USE_CACHE)" = "true" ]; then \
		mkdir -p $(CACHE_PATH); \
		gsutil -q -m cp gs://$(GCS_BUCKET)/$(1)/cache_$(ARCHITECTURE)_$(BUILD_TYPE_SANITIZERS)_$(1).tar.zst \
			$(call sanitizer_cache_file,$(1)) || true; \
		$(call extract_sanitizer_cache,$(1)) || true; \
	else \
		echo "-- Cache usage disabled."; \
	fi
endef

# Check if sanitizer cache should be uploaded
define check_and_upload_sanitizer_cache
	@echo "-- Checking if $(1) cache upload is allowed..."
	if [ "$(USE_CACHE)" = "true" ] && [ "$(CAN_UPLOAD_CACHE)" = "true" ]; then \
		echo "-- Preparing $(1) cache upload..."; \
		$(call generate_sanitizer_cache,$(1)); \
		echo "-- Uploading $(1) cache to GCS..."; \
		mkdir -p $(CACHE_PATH)/$(1); \
		gsutil -q -m cp $(call sanitizer_cache_file,$(1)) gs://$(GCS_BUCKET)/$(1)/cache_$(ARCHITECTURE)_$(BUILD_TYPE_SANITIZERS)_$(1).tar.zst; \
		echo "-- $(1) cache uploaded."; \
	else \
		echo "-- $(1) cache upload not allowed or disabled."; \
	fi
endef

# Sanitizer cache handling for each sanitizer type
kagome_cache_get_tsan:
	$(call get_sanitizer_cache,tsan)

kagome_cache_get_asan:
	$(call get_sanitizer_cache,asan)

kagome_cache_get_ubsan:
	$(call get_sanitizer_cache,ubsan)

kagome_cache_get_asanubsan:
	$(call get_sanitizer_cache,asanubsan)

kagome_cache_check_and_upload_tsan:
	$(call check_and_upload_sanitizer_cache,tsan)

kagome_cache_check_and_upload_asan:
	$(call check_and_upload_sanitizer_cache,asan)

kagome_cache_check_and_upload_ubsan:
	$(call check_and_upload_sanitizer_cache,ubsan)

kagome_cache_check_and_upload_asanubsan:
	$(call check_and_upload_sanitizer_cache,asanubsan)

# Target for getting all sanitizer caches
kagome_cache_get_all_sanitizers: kagome_cache_get_asan kagome_cache_get_tsan kagome_cache_get_ubsan kagome_cache_get_asanubsan
	@echo "-- All sanitizer caches retrieved."

# Target for uploading all sanitizer caches
kagome_cache_upload_all_sanitizers: kagome_cache_check_and_upload_asan kagome_cache_check_and_upload_tsan kagome_cache_check_and_upload_ubsan kagome_cache_check_and_upload_asanubsan
	@echo "-- All sanitizer caches uploaded."
