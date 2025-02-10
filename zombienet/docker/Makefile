# Check if the version files exist before running any targets
VERSION_FILES := commit_hash.txt kagome_version.txt polkadot-sdk-versions.txt zombienet-versions.txt
ifneq ($(filter configure,$(MAKECMDGOALS)),configure)
missing_files := $(filter-out $(wildcard $(VERSION_FILES)),$(VERSION_FILES))
ifneq ($(missing_files),)
  $(error Missing version files: $(missing_files). Use 'make configure' to generate them.)
endif

# Check if the necessary environment variables are set before running any targets
ENV_VARS := DOCKER_REGISTRY_PATH PROJECT_ID GOOGLE_APPLICATION_CREDENTIALS
missing_env := $(strip $(foreach var,$(ENV_VARS),$(if $(strip $($(var))),, $(var))))
ifneq ($(missing_env),)
  $(error Missing environment variables: $(missing_env). Please set them before running make.)
endif
endif

# Include all the necessary files
include Makefile.include/variables.mk
include Makefile.include/run_zombietest.mk
include Makefile.include/polkadot.mk
include Makefile.include/zombie_tester.mk
include Makefile.include/zombietests.mk
include Makefile.include/versions.mk

# Run the configure target to generate the version files
configure: get_versions

.PHONY: configure
