ifeq ($(filter configure,$(MAKECMDGOALS)),)



endif

get_versions:
	@echo "full_commit_hash: `git rev-parse HEAD`" | tee commit_hash.txt
	@echo "short_commit_hash: `git rev-parse HEAD | head -c 7`" | tee -a commit_hash.txt
	@echo "kagome_version: `cd $(WORKING_DIR) && ./get_version.sh`" | tee kagome_version.txt
	@echo "kagome_sanitized_version: $(KAGOME_SANITIZED_VERSION)" | tee -a kagome_version.txt

.PHONY: get_versions 
