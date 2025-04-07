ifeq ($(filter configure,$(MAKECMDGOALS)),)

ifeq ($(KAGOME_DEB_PACKAGE_VERSION_NO_ARCH),)
KAGOME_DEB_PACKAGE_VERSION_NO_ARCH=$(CURRENT_DATE)-$(SHORT_COMMIT_HASH)-$(BUILD_TYPE)
endif

ifeq ($(KAGOME_DEB_PACKAGE_VERSION),)
KAGOME_DEB_PACKAGE_VERSION=$(KAGOME_DEB_PACKAGE_VERSION_NO_ARCH)-$(ARCHITECTURE)
endif

ifeq ($(KAGOME_DEB_RELEASE_PACKAGE_VERSION),)
	KAGOME_DEB_RELEASE_PACKAGE_VERSION=$(KAGOME_SANITIZED_VERSION)-$(BUILD_TYPE)-$(ARCHITECTURE)
endif

KAGOME_DEB_PACKAGE_NAME=kagome-dev_$(KAGOME_DEB_PACKAGE_VERSION)_$(ARCHITECTURE).deb
KAGOME_DEB_RELEASE_PACKAGE_NAME=kagome_$(KAGOME_DEB_RELEASE_PACKAGE_VERSION)_$(ARCHITECTURE).deb

endif

get_versions:
	@echo "full_commit_hash: `git rev-parse HEAD`" | tee commit_hash.txt
	@echo "short_commit_hash: `git rev-parse HEAD | head -c 7`" | tee -a commit_hash.txt
	@echo "kagome_version: `cd $(WORKING_DIR) && ./get_version.sh`" | tee kagome_version.txt
	@echo "kagome_sanitized_version: $(KAGOME_SANITIZED_VERSION)" | tee -a kagome_version.txt

.PHONY: get_versions
