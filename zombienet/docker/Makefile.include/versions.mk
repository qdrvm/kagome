get_versions:
	python3 version.py https://github.com/paritytech/polkadot-sdk $(POLKADOT_SDK_TAG) && \
	python3 version_sem.py https://github.com/paritytech/zombienet
	@echo "full_commit_hash: `git rev-parse HEAD`" | tee commit_hash.txt
	@echo "short_commit_hash: `git rev-parse HEAD | head -c 7`" | tee -a commit_hash.txt
	@echo "kagome_version: `cd $(WORKING_DIR) && ./get_version.sh`" | tee kagome_version.txt

set_versions: get_versions
ifeq ($(POLKADOT_SDK_RELEASE),)
$(eval POLKADOT_SDK_RELEASE=$(shell grep 'polkadot_format_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2))
endif
export POLKADOT_SDK_RELEASE
ifeq ($(ZOMBIENET_RELEASE),)
$(eval ZOMBIENET_RELEASE=$(shell grep 'numeric_version:' zombienet-versions.txt | cut -d ' ' -f 2))
endif
export ZOMBIENET_RELEASE
POLKADOT_RELEASE_GLOBAL_NUMERIC=$(shell echo $(POLKADOT_SDK_RELEASE) | sed 's/[^0-9]//g' )
export POLKADOT_RELEASE_GLOBAL_NUMERIC
ifeq ($(POLKADOT_DEB_PACKAGE_VERSION_NO_ARCH),)
POLKADOT_DEB_PACKAGE_VERSION_NO_ARCH=$(CURRENT_DATE)-$(POLKADOT_RELEASE_GLOBAL_NUMERIC)
endif
export POLKADOT_DEB_PACKAGE_VERSION_NO_ARCH
ifeq ($(POLKADOT_DEB_PACKAGE_VERSION),)
POLKADOT_DEB_PACKAGE_VERSION=$(POLKADOT_DEB_PACKAGE_VERSION_NO_ARCH)-$(ARCHITECTURE)
endif
export POLKADOT_DEB_PACKAGE_VERSION
POLKADOT_DEB_PACKAGE_NAME=polkadot-binary_$(POLKADOT_DEB_PACKAGE_VERSION)_$(ARCHITECTURE).deb
export POLKADOT_DEB_PACKAGE_NAME

.PHONY: get_versions set_versions
