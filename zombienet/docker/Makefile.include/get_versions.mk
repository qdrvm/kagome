get_versions:
	python3 version.py https://github.com/paritytech/polkadot-sdk $(POLKADOT_SDK_TAG) && \
	python3 version_sem.py https://github.com/paritytech/zombienet
	@echo "full_commit_hash: `git rev-parse HEAD`" | tee commit_hash.txt
	@echo "short_commit_hash: `git rev-parse HEAD | head -c 7`" | tee -a commit_hash.txt
	@echo "kagome_version: `cd $(WORKING_DIR) && ./get_version.sh`" | tee kagome_version.txt
