# Sanitizer-specific cache directories and functions

# Define sanitizer cache directories and files
define sanitizer_cache_dir
$(call sanitizer_build_dir,$(1))/cache
endef

define sanitizer_cache_tag
$(CURRENT_DATE)-$(HOST_OS)-$(ARCHITECTURE)-$(BUILD_TYPE_SANITIZERS)-$(1)
endef

define sanitizer_cache_archive
build-cache-$(call sanitizer_cache_tag,$(1)).tar.zst
endef

# Cache management functions for sanitizers
define sanitizer_cache_upload
	if [ "$(CACHE_UPLOAD_ALLOWED)" = "true" ]; then \
		echo "-- Preparing $(1) cache archive..."; \
		cd $(WORKING_DIR); \
		SANITIZER_BUILD_DIR=build_docker_$(ARCHITECTURE)_$(BUILD_TYPE_SANITIZERS)_$(1); \
		SANITIZER_CACHE_ARCHIVE=$(call sanitizer_cache_archive,$(1)); \
		$(call measure_time,tar cf - \
			--exclude="$$SANITIZER_BUILD_DIR/node/kagome" \
			--exclude="$$SANITIZER_BUILD_DIR/kagome_binary" \
			--exclude="$$SANITIZER_BUILD_DIR/pkg" \
			$$SANITIZER_BUILD_DIR | \
			zstd -$(ZSTD_LEVEL) -T$(ZSTD_THREADS) > "$$SANITIZER_CACHE_ARCHIVE") && \
		echo "-- Uploading $(1) cache to gs://$(CACHE_BUCKET)/" && \
		$(call measure_time,gcloud storage cp "$$SANITIZER_CACHE_ARCHIVE" "gs://$(CACHE_BUCKET)/") && \
		rm -f "$$SANITIZER_CACHE_ARCHIVE" && \
		echo "-- $(1) cache uploaded successfully"; \
	else \
		echo "-- $(1) cache upload not allowed (not on main branch or cache too recent)"; \
	fi
endef

define sanitizer_cache_get
	echo "-- Searching for $(1) cache in gs://$(CACHE_BUCKET)..." ; \
	SANITIZER_CACHE_TAG=$(call sanitizer_cache_tag,$(1)); \
	FOUND_CACHE=$$(gcloud storage ls "gs://$(CACHE_BUCKET)/build-cache-*-$(HOST_OS)-$(ARCHITECTURE)-$(BUILD_TYPE_SANITIZERS)-$(1).tar.zst" 2>/dev/null | sort -r | head -n1); \
	if [ -n "$$FOUND_CACHE" ]; then \
		echo "-- Found suitable $(1) cache: $$FOUND_CACHE"; \
		mkdir -p $(call sanitizer_build_dir,$(1)) && \
		cd $(WORKING_DIR); \
		SANITIZER_CACHE_ARCHIVE=$(call sanitizer_cache_archive,$(1)); \
		echo "-- Downloading $(1) cache..."; \
		$(call measure_time,gcloud storage cp "$$FOUND_CACHE" "$$SANITIZER_CACHE_ARCHIVE") && \
		echo "-- Extracting $(1) cache archive..." && \
		$(call measure_time,zstd -d "$$SANITIZER_CACHE_ARCHIVE" --stdout | tar xf -) && \
		rm -f "$$SANITIZER_CACHE_ARCHIVE" && \
		echo "-- $(1) cache downloaded and extracted successfully"; \
	else \
		echo "-- No suitable $(1) cache found"; \
	fi
endef

define sanitizer_cache_check_age
	NEWEST_CACHE=$$(gcloud storage ls "gs://$(CACHE_BUCKET)/build-cache-*-$(HOST_OS)-$(ARCHITECTURE)-$(BUILD_TYPE_SANITIZERS)-$(1).tar.zst" 2>/dev/null | sort -r | head -n1); \
	if [ -n "$$NEWEST_CACHE" ]; then \
		CACHE_DATE=$$(echo "$$NEWEST_CACHE" | sed -n 's/.*build-cache-\([0-9]\{8\}\).*/\1/p'); \
		DAYS_OLD=$$(( ( $$(date -d "$(CURRENT_DATE)" +%s) - $$(date -d "$$CACHE_DATE" +%s) ) / 86400 )); \
		[ $$DAYS_OLD -gt $(CACHE_LIFETIME_DAYS) ]; \
	else \
		true; \
	fi
endef

define sanitizer_cache_check_and_upload
    echo "-- Checking $(1) cache upload conditions..." ; \
    if [ "$(CACHE_UPLOAD_ALLOWED)" = "false" ] && { [ "$(CACHE_ONLY_MASTER)" = "false" ] || [ "$(GIT_REF_NAME)" = "master" ]; }; then \
        if $(call sanitizer_cache_check_age,$(1)); then \
            echo "-- $(1) cache is older than $(CACHE_LIFETIME_DAYS) days or missing - enabling upload"; \
            $(MAKE) kagome_cache_upload_$(1) CACHE_UPLOAD_ALLOWED=true; \
        else \
            NEWEST_CACHE=$$(gcloud storage ls "gs://$(CACHE_BUCKET)/build-cache-*-$(HOST_OS)-$(ARCHITECTURE)-$(BUILD_TYPE_SANITIZERS)-$(1).tar.zst" 2>/dev/null | sort -r | head -n1); \
            echo "-- $(1) cache is fresh (< $(CACHE_LIFETIME_DAYS)d) - skipping upload"; \
            echo "-- Latest $(1) cache: $$NEWEST_CACHE"; \
        fi; \
    else \
        if [ "$(CACHE_UPLOAD_ALLOWED)" = "true" ]; then \
            echo "-- $(1) cache upload is already allowed - proceeding with upload"; \
            $(call sanitizer_cache_upload,$(1)); \
        else \
            echo "-- $(1) cache upload not allowed (not on master branch)"; \
        fi; \
    fi
endef

# Cache management targets for sanitizers
kagome_cache_upload_asan:
	$(call sanitizer_cache_upload,asan)

kagome_cache_upload_tsan:
	$(call sanitizer_cache_upload,tsan)

kagome_cache_upload_ubsan:
	$(call sanitizer_cache_upload,ubsan)

kagome_cache_upload_asanubsan:
	$(call sanitizer_cache_upload,asanubsan)

kagome_cache_get_asan:
	$(call sanitizer_cache_get,asan)

kagome_cache_get_tsan:
	$(call sanitizer_cache_get,tsan)

kagome_cache_get_ubsan:
	$(call sanitizer_cache_get,ubsan)

kagome_cache_get_asanubsan:
	$(call sanitizer_cache_get,asanubsan)

kagome_cache_check_and_upload_asan:
	$(call sanitizer_cache_check_and_upload,asan)

kagome_cache_check_and_upload_tsan:
	$(call sanitizer_cache_check_and_upload,tsan)

kagome_cache_check_and_upload_ubsan:
	$(call sanitizer_cache_check_and_upload,ubsan)

kagome_cache_check_and_upload_asanubsan:
	$(call sanitizer_cache_check_and_upload,asanubsan)

# Display sanitizer cache info
sanitizer_cache_info:
	@echo "Sanitizer Cache Configuration:"
	@echo "  ASAN Cache Tag: $(call sanitizer_cache_tag,asan)"
	@echo "  TSAN Cache Tag: $(call sanitizer_cache_tag,tsan)" 
	@echo "  UBSAN Cache Tag: $(call sanitizer_cache_tag,ubsan)"
	@echo "  ASANUBSAN Cache Tag: $(call sanitizer_cache_tag,asanubsan)"
	@echo "  Cache lifetime: $(CACHE_LIFETIME_DAYS) days"
	@echo "  Upload allowed: $(CACHE_UPLOAD_ALLOWED)"
	@echo "  Cache bucket: gs://$(CACHE_BUCKET)/"

sanitizer_cache_list:
	@echo "Available sanitizer caches in gs://$(CACHE_BUCKET)/:"
	@gcloud storage ls "gs://$(CACHE_BUCKET)/build-cache-*-$(HOST_OS)-$(ARCHITECTURE)-$(BUILD_TYPE_SANITIZERS)-*.tar.zst" 2>/dev/null || echo "No sanitizer caches found"

.PHONY: kagome_cache_upload_asan kagome_cache_upload_tsan kagome_cache_upload_ubsan kagome_cache_upload_asanubsan \
        kagome_cache_get_asan kagome_cache_get_tsan kagome_cache_get_ubsan kagome_cache_get_asanubsan \
        kagome_cache_check_and_upload_asan kagome_cache_check_and_upload_tsan kagome_cache_check_and_upload_ubsan kagome_cache_check_and_upload_asanubsan \
        sanitizer_cache_info sanitizer_cache_list

