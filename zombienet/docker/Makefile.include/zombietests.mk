test-polkadot-functional-0001-parachains-pvf:
	$(call run_test, "kagome/zombienet/polkadot/functional/0001-parachains-pvf.zndsl")

test-polkadot-functional-0002-parachains-disputes:
	$(call run_test, "kagome/zombienet/polkadot/functional/0002-parachains-disputes.zndsl")

test-polkadot-functional-0003-beefy-and-mmr:
	$(call run_test, "kagome/zombienet/polkadot/functional/0003-beefy-and-mmr.zndsl")

test-polkadot-functional-0004-parachains-garbage-candidate:
	$(call run_test, "kagome/zombienet/polkadot/functional/0004-parachains-garbage-candidate.zndsl")

test-polkadot-functional-0005-parachains-disputes-past-session:
	$(call run_test, "kagome/zombienet/polkadot/functional/0005-parachains-disputes-past-session.zndsl")

test-polkadot-functional-0006-parachains-max-tranche0:
	$(call run_test, "kagome/zombienet/polkadot/functional/0006-parachains-max-tranche0.zndsl")

test-polkadot-functional-0007-dispute-freshly-finalized:
	$(call run_test, "kagome/zombienet/polkadot/functional/0007-dispute-freshly-finalized.zndsl")

test-polkadot-functional-0008-dispute-old-finalized:
	$(call run_test, "kagome/zombienet/polkadot/functional/0008-dispute-old-finalized.zndsl")

test-polkadot-functional-0009-approval-voting-coalescing:
	$(call run_test, "kagome/zombienet/polkadot/functional/0009-approval-voting-coalescing.zndsl")

test-polkadot-functional-0010-validator-disabling:
	$(call run_test, "kagome/zombienet/polkadot/functional/0010-validator-disabling.zndsl")

test-polkadot-functional-0011-async-backing-6-seconds-rate:
	$(call run_test, "kagome/zombienet/polkadot/functional/0011-async-backing-6-seconds-rate.zndsl")

test-polkadot-functional-0013-systematic-chunk-recovery:
	$(call run_test, "kagome/zombienet/polkadot/functional/0013-systematic-chunk-recovery.zndsl")

test-custom-0001-validators-warp-sync:
	$(call run_test, "kagome/zombienet/custom/0001-validators-warp-sync.zndsl")
