# Check if the version files exist before running any targets
VERSION_FILES := commit_hash.txt kagome_version.txt
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

define safe_rm
    PATH_TO_RM="$$(readlink -f '$(1)')"; \
    for F in / /home /etc /usr; do \
        if [ "$$PATH_TO_RM" = "$$F" ]; then \
            echo "Forbidden path: $$PATH_TO_RM"; \
            exit 1; \
        fi; \
    done; \
    echo "Checking $$PATH_TO_RM..."; \
    if [ -d "$$PATH_TO_RM" ]; then \
        echo "Deleting $$PATH_TO_RM"; \
        rm -rv --preserve-root "$$PATH_TO_RM"; \
    else \
        echo "Directory $$PATH_TO_RM does not exist"; \
    fi
endef

# Include all the necessary files
include Makefile.include/variables.mk
include Makefile.include/versions.mk
include Makefile.include/builder.mk
include Makefile.include/kagome_bin.mk
include Makefile.include/kagome_image.mk
include Makefile.include/runtime.mk
include Makefile.include/sanitizers.mk
include Makefile.include/tidy.mk

# Run the configure target to generate the version files
configure: get_versions

.PHONY: configure
