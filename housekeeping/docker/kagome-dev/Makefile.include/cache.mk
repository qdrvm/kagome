cache_info:
	@echo "Cache configuration:"
	@echo "  Tag: $(CACHE_TAG)"
	@echo "  Upload allowed: $(CACHE_UPLOAD_ALLOWED)"
	@echo "  Cache lifetime: $(CACHE_LIFETIME_HOURS) hours"
	@echo "  Cache bucket: gs://$(CACHE_BUCKET)"
	@echo "  Cache archive: $(CACHE_ARCHIVE)"
	@echo "  Compression level: $(ZSTD_LEVEL)"
	@echo "  Compression threads: $(ZSTD_THREADS)"

cache_list:
	@echo "Available caches in gs://$(CACHE_BUCKET)/:"
	@gcloud storage ls "gs://$(CACHE_BUCKET)/" || true

define measure_time
    START_TIME=$$(date +%s); \
    $(1); \
    EXIT_CODE=$$?; \
    END_TIME=$$(date +%s); \
    echo "Time elapsed: $$(($$END_TIME - $$START_TIME)) seconds"; \
    [ $$EXIT_CODE -eq 0 ]
endef

cache_upload:
	@if [ "$(CACHE_UPLOAD_ALLOWED)" = "true" ]; then \
		echo "Preparing cache archive..."; \
		cd $(WORKING_DIR); \
		$(call measure_time,tar cf - \
			--exclude="$(DOCKER_BUILD_DIR_NAME)/node/kagome" \
			--exclude="$(DOCKER_BUILD_DIR_NAME)/kagome_binary" \
			--exclude="$(DOCKER_BUILD_DIR_NAME)/pkg" \
			$(DOCKER_BUILD_DIR_NAME) | \
			zstd -$(ZSTD_LEVEL) -T$(ZSTD_THREADS) > "$(CACHE_ARCHIVE)") && \
		echo "Uploading cache to gs://$(CACHE_BUCKET)/" && \
		$(call measure_time,gcloud storage cp "$(CACHE_ARCHIVE)" "gs://$(CACHE_BUCKET)/") && \
		rm -f "$(CACHE_ARCHIVE)" && \
		echo "Cache uploaded successfully"; \
	else \
		echo "Cache upload not allowed (not on main branch or cache too recent)"; \
		exit 1; \
	fi

cache_get:
	@echo "Searching for $(BUILD_TYPE) cache in gs://$(CACHE_BUCKET)..."
	@FOUND_CACHE=$$(gcloud storage ls "gs://$(CACHE_BUCKET)/build-cache-*-$(HOST_OS)-$(ARCHITECTURE)-$(BUILD_TYPE).tar.zst" 2>/dev/null | sort -r | head -n1); \
	if [ -n "$$FOUND_CACHE" ]; then \
		echo "Found suitable $(BUILD_TYPE) cache: $$FOUND_CACHE"; \
		mkdir -p $(WORKING_DIR)/$(DOCKER_BUILD_DIR_NAME) && \
		cd $(WORKING_DIR); \
		echo "Downloading cache..."; \
		$(call measure_time,gcloud storage cp "$$FOUND_CACHE" "$(CACHE_ARCHIVE)") && \
		echo "Extracting cache archive..." && \
		$(call measure_time,zstd -d "$(CACHE_ARCHIVE)" --stdout | tar xf -) && \
		rm -f "$(CACHE_ARCHIVE)" && \
		echo "Cache downloaded and extracted successfully"; \
	else \
		echo "No suitable $(BUILD_TYPE) cache found"; \
		exit 1; \
	fi

define check_cache_age
    NEWEST_CACHE=$$(gcloud storage ls "gs://$(CACHE_BUCKET)/build-cache-*-$(HOST_OS)-$(ARCHITECTURE)-$(BUILD_TYPE).tar.zst" 2>/dev/null | sort -r | head -n1); \
    if [ -n "$$NEWEST_CACHE" ]; then \
        CACHE_DATE=$$(echo "$$NEWEST_CACHE" | sed -n 's/.*build-cache-\([0-9]\{8\}\).*/\1/p'); \
        DAYS_OLD=$$(( ( $$(date -d "$(CURRENT_DATE)" +%s) - $$(date -d "$$CACHE_DATE" +%s) ) / 86400 )); \
        [ $$DAYS_OLD -gt $(CACHE_LIFETIME_DAYS) ]; \
    else \
        true; \
    fi
endef

cache_check_and_upload:
	@echo "Checking cache upload conditions..."
	@if [ "$(CACHE_UPLOAD_ALLOWED)" = "false" ] && ( [ "$(CACHE_ONLY_MASTER)" = "false" ] || [ "$(GIT_REF_NAME)" = "master" ] ); then \
		if $(call check_cache_age); then \
			echo "Cache is older than $(CACHE_LIFETIME_DAYS) days or missing - enabling upload"; \
			$(MAKE) cache_upload CACHE_UPLOAD_ALLOWED=true; \
		else \
			NEWEST_CACHE=$$(gcloud storage ls "gs://$(CACHE_BUCKET)/build-cache-*-$(HOST_OS)-$(ARCHITECTURE)-$(BUILD_TYPE).tar.zst" 2>/dev/null | sort -r | head -n1); \
			echo "Cache is fresh (< $(CACHE_LIFETIME_DAYS)d) - skipping upload"; \
			echo "Latest cache: $$NEWEST_CACHE"; \
			exit 0; \
		fi; \
	else \
		if [ "$(CACHE_UPLOAD_ALLOWED)" = "true" ]; then \
			echo "Cache upload is already allowed - proceeding with upload"; \
			$(MAKE) cache_upload; \
		else \
			echo "Cache upload not allowed (not on master branch)"; \
			exit 0; \
		fi; \
	fi
