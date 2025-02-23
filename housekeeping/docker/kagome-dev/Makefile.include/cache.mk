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
	if [ "$(CACHE_UPLOAD_ALLOWED)" = "true" ]; then \
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
	if [ -n "$$(gcloud storage ls gs://$(CACHE_BUCKET)/$(CACHE_ARCHIVE) 2>/dev/null)" ]; then \
		echo "Downloading cache from gs://$(CACHE_BUCKET)/$(CACHE_ARCHIVE)"; \
		cd $(WORKING_DIR); \
		$(call measure_time,gcloud storage cp "gs://$(CACHE_BUCKET)/$(CACHE_ARCHIVE)" "$(CACHE_ARCHIVE)") && \
		echo "Extracting cache archive..." && \
		$(call measure_time,zstd -d "$(CACHE_ARCHIVE)" --stdout | tar xf -) && \
		rm -f "$(CACHE_ARCHIVE)" && \
		echo "Cache downloaded and extracted successfully"; \
	else \
		echo "Cache not found in gs://$(CACHE_BUCKET)/$(CACHE_ARCHIVE)"; \
		exit 1; \
	fi
