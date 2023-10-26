/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_PROSPECTIVE_PARACHAINS_HPP
#define KAGOME_PARACHAIN_PROSPECTIVE_PARACHAINS_HPP

#include <boost/variant.hpp>
#include <map>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "network/types/collator_messages_vstaging.hpp"
#include "parachain/types.hpp"
#include "parachain/validator/collations.hpp"
#include "parachain/validator/fragment_tree.hpp"
#include "utils/map.hpp"
#include "blockchain/block_tree_error.hpp"

namespace kagome::parachain {

  class ProspectiveParachains {
    struct RelayBlockViewData {
      // Scheduling info for paras and upcoming paras.
      std::unordered_map<ParachainId, fragment::FragmentTree> fragment_trees;
      std::unordered_set<CandidateHash> pending_availability;
    };

    struct View {
      // Active or recent relay-chain blocks by block hash.
      std::unordered_map<Hash, RelayBlockViewData> active_leaves;
      std::unordered_map<ParachainId, fragment::CandidateStorage>
          candidate_storage;
    };

    struct ImportablePendingAvailability {
      network::CommittedCandidateReceipt candidate;
      runtime::PersistedValidationData persisted_validation_data;
      fragment::PendingAvailability compact;
    };

    View view;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<runtime::ParachainHost> parachain_host_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    log::Logger logger =
        log::createLogger("ProspectiveParachains", "parachain");

   public:
    ProspectiveParachains(std::shared_ptr<crypto::Hasher> hasher, std::shared_ptr<runtime::ParachainHost> parachain_host, std::shared_ptr<blockchain::BlockTree> block_tree)
        : hasher_{std::move(hasher)}, parachain_host_{std::move(parachain_host)}, block_tree_{std::move(block_tree)} {
          BOOST_ASSERT(hasher_);
          BOOST_ASSERT(parachain_host_);
          BOOST_ASSERT(block_tree_);
        }

    std::optional<runtime::PersistedValidationData>
    answerProspectiveValidationDataRequest(
        const RelayHash &candidate_relay_parent,
        const Hash &parent_head_data_hash,
        ParachainId para_id) {
      if (auto it = view.candidate_storage.find(para_id);
          it != view.candidate_storage.end()) {
        fragment::CandidateStorage &storage = it->second;
        std::optional<HeadData> head_data =
            utils::fromRefToOwn(storage.headDataByHash(parent_head_data_hash));
        std::optional<fragment::RelayChainBlockInfo> relay_parent_info{};
        std::optional<size_t> max_pov_size{};

        for (const auto &[_, x] : view.active_leaves) {
          auto it = x.fragment_trees.find(para_id);
          if (it == x.fragment_trees.end()) {
            continue;
          }
          const fragment::FragmentTree &fragment_tree = it->second;

          if (head_data && relay_parent_info && max_pov_size) {
            break;
          }
          if (!relay_parent_info) {
            relay_parent_info = utils::fromRefToOwn(
                fragment_tree.scope.ancestorByHash(candidate_relay_parent));
          }
          if (!head_data) {
            const auto &required_parent =
                fragment_tree.scope.base_constraints.required_parent;
            if (crypto::Hashed<const HeadData &, 32>{required_parent}.getHash()
                == parent_head_data_hash) {
              head_data = required_parent;
            }
          }
          if (!max_pov_size) {
            if (fragment_tree.scope.ancestorByHash(candidate_relay_parent)) {
              max_pov_size = fragment_tree.scope.base_constraints.max_pov_size;
            }
          }
        }

        if (head_data && relay_parent_info && max_pov_size) {
          return runtime::PersistedValidationData{
            .parent_head = *head_data,
            .relay_parent_number = relay_parent_info->number,
            .relay_parent_storage_root = relay_parent_info->storage_root,
            .max_pov_size = (uint32_t)*max_pov_size,
          };
        }
      }
      return std::nullopt;
    }

    std::optional<ProspectiveParachainsMode> prospectiveParachainsMode(const RelayHash &relay_parent) {
      /// request runtime for prospective parachains mode
      /// TODO(iceseer): do
      return std::nullopt;
    }

outcome::result<std::optional<std::pair<fragment::Constraints, std::vector<network::vstaging::CandidatePendingAvailability>>>> fetchBackingState(const RelayHash &relay_parent, ParachainId para_id) {
      /// TODO(iceseer): do

//	let (tx, rx) = oneshot::channel();
//	ctx.send_message(RuntimeApiMessage::Request(
//		relay_parent,
//		RuntimeApiRequest::StagingParaBackingState(para_id, tx),
//	))
//	.await;
//
//	Ok(rx
//		.await
//		.map_err(JfyiError::RuntimeApiRequestCanceled)??
//		.map(|s| (From::from(s.constraints), s.pending_availability)))

      return std::nullopt;
}

    outcome::result<std::optional<fragment::RelayChainBlockInfo>> fetchBlockInfo(const RelayHash &relay_hash) {
      /// TODO(iceseer): do
      /// cache for block header request and calculations
      auto res_header = block_tree_->getBlockHeader(relay_hash);
      if (res_header.has_error()) {
        if (res_header.error() == blockchain::BlockTreeError::HEADER_NOT_FOUND) {
          return outcome::success(std::nullopt);
        }
        return res_header.error();
      }

      return fragment::RelayChainBlockInfo {
        .hash = relay_hash,
        .number = res_header.value().number,
        .storage_root = res_header.value().state_root,
      };
    }

outcome::result<std::unordered_set<ParachainId>> fetchUpcomingParas(const RelayHash &relay_parent, std::unordered_set<CandidateHash> &pending_availability) {
  OUTCOME_TRY(cores, parachain_host_->availability_cores(relay_parent));

  std::unordered_set<ParachainId> upcoming;
  for (const auto &core : cores) {
    visit_in_place(core, [&](const runtime::OccupiedCore &occupied) {
      pending_availability.insert(occupied.candidate_hash);
				if (occupied.next_up_on_available) {
					upcoming.insert(occupied.next_up_on_available->para_id);
				}
				if (occupied.next_up_on_time_out) {
					upcoming.insert(occupied.next_up_on_time_out->para_id);
				}
    }, [&](const runtime::ScheduledCore &scheduled) {
      upcoming.insert(scheduled.para_id);
    }, [](const auto &) {});
  }
  return upcoming;
}

outcome::result<std::vector<fragment::RelayChainBlockInfo>> fetchAncestry(const RelayHash &relay_hash, size_t ancestors) {
  std::vector<fragment::RelayChainBlockInfo> block_info;
	if (ancestors == 0) {
		return block_info;
	}

  OUTCOME_TRY(hashes, block_tree_->getDescendingChainToBlock(relay_hash, ancestors));
  OUTCOME_TRY(required_session, parachain_host_->session_index_for_child(relay_hash));
  
  block_info.reserve(hashes.size());
  for (const auto &hash : hashes) {
    OUTCOME_TRY(info, fetchBlockInfo(hash));
    if (!info) {
      SL_WARN(logger, "Failed to fetch info for hash returned from ancestry. (relay_hash={})", hash);
      break;
    }
    OUTCOME_TRY(session, parachain_host_->session_index_for_child(hash));
		if (session == required_session) {
			block_info.emplace_back(*info);
		} else {
			break;
		}
  }
  return block_info;
}

outcome::result<std::vector<ImportablePendingAvailability>> preprocessCandidatesPendingAvailability(const HeadData &required_parent, const std::vector<network::vstaging::CandidatePendingAvailability> &pending_availability) {
    std::reference_wrapper<const HeadData> required_parent_copy = required_parent;
    std::vector<ImportablePendingAvailability> importable;
    const size_t expected_count = pending_availability.size();

    for (size_t i = 0; i < pending_availability.size(); i++) {
        const auto &pending = pending_availability[i];
        OUTCOME_TRY(relay_parent, fetchBlockInfo(pending.descriptor.relay_parent));
        if (!relay_parent) {
          SL_DEBUG(logger, "Had to stop processing pending candidates early due to missing info. (candidate hash={}, parachain id={}, index={}, expected count={})", 
            pending.candidate_hash, pending.descriptor.para_id, i, expected_count);
            break;
        }

        const fragment::RelayChainBlockInfo &b = *relay_parent;
        importable.push_back(ImportablePendingAvailability {
            network::CommittedCandidateReceipt {
                pending.descriptor,
                pending.commitments,
            },
            runtime::PersistedValidationData {
                required_parent_copy.get(),
                b.number,
                b.storage_root,
                pending.max_pov_size,
            },
            fragment::PendingAvailability {
                pending.candidate_hash,
                b,
            }
        });
        required_parent_copy = pending.commitments.para_head;
    }
    return importable;
}

outcome::result<void> onActiveLeavesUpdate(const network::ExView &update) {
    /// TODO(iceseer): do
    /// call from parachain_processor onActiveLeavesUpdate

    for (const auto& deactivated : update.lost) {
        view.active_leaves.erase(deactivated);
    }
    std::unordered_map<Hash, ProspectiveParachainsMode> temp_header_cache;
    const auto& activated = update.new_head;
    const auto& hash = primitives::calculateBlockHash(update.new_head, *hasher_).value();
    const auto mode = prospectiveParachainsMode(hash);
    if (!mode) {
      SL_TRACE(logger, "Skipping leaf activation since async backing is disabled. (block_hash={})", hash);
        return outcome::success();
    }
    std::unordered_set<Hash> pending_availability{};
    OUTCOME_TRY(scheduled_paras, fetchUpcomingParas(hash, pending_availability));

    const fragment::RelayChainBlockInfo block_info {
      .hash = hash,
      .number = activated.number,
      .storage_root= activated.state_root,
    };

    OUTCOME_TRY(ancestry, fetchAncestry(hash, mode->allowed_ancestry_len));
    std::unordered_map<ParachainId, fragment::FragmentTree> fragment_trees;
    for (ParachainId para : scheduled_paras) {
      auto &candidate_storage = view.candidate_storage[para];
      OUTCOME_TRY(backing_state, fetchBackingState(hash, para));
      if (!backing_state) {
        SL_TRACE(logger, "Failed to get inclusion backing state. (para={}, relay parent={})", para, hash);
        continue;
      }
      const auto &[constraints, pe] = *backing_state;
      OUTCOME_TRY(pending_availability, preprocessCandidatesPendingAvailability(constraints.required_parent, pe));

      std::vector<fragment::PendingAvailability> compact_pending;
      compact_pending.reserve(pending_availability.size());

      for (const ImportablePendingAvailability &c : pending_availability) {
				const auto &candidate_hash = c.compact.candidate_hash;
        auto res = candidate_storage.addCandidate(candidate_hash, c.candidate, crypto::Hashed<const runtime::PersistedValidationData &, 32>{c.persisted_validation_data}, hasher_);
				compact_pending.emplace_back(c.compact);

        if (res.has_value() || res.error() == fragment::CandidateStorage::Error::CANDIDATE_ALREADY_KNOWN) {
          candidate_storage.markBacked(candidate_hash);
        } else {
          SL_WARN(logger, "Scraped invalid candidate pending availability. (candidate_hash={}, para={}, error={})",
          candidate_hash, para, res.error().message());
        }
      }

      OUTCOME_TRY(scope, fragment::Scope::withAncestors(
				para,
				block_info,
				constraints,
				compact_pending,
				mode->max_candidate_depth,
				ancestry
			));
      fragment_trees.emplace(para, fragment::FragmentTree::populate(scope, candidate_storage));
    }

		view.active_leaves.emplace(hash, RelayBlockViewData { 
      fragment_trees, 
      pending_availability 
      });

    if (!update.lost.empty()) {
      /// TODO(iceseer): do
      /// prune_view_candidate_storage
    }

    return outcome::success();
}

    /// @brief calculates hypothetical candidate and fragment tree membership
    /// @param candidates Candidates, in arbitrary order, which should be
    /// checked for possible membership in fragment trees.
    /// @param fragment_tree_relay_parent Either a specific fragment tree to
    /// check, otherwise all.
    /// @param backed_in_path_only Only return membership if all candidates in
    /// the path from the root are backed.
    std::vector<
        std::pair<HypotheticalCandidate, fragment::FragmentTreeMembership>>
    answerHypotheticalFrontierRequest(
        const gsl::span<const HypotheticalCandidate> &candidates,
        const std::optional<std::reference_wrapper<const Hash>>
            &fragment_tree_relay_parent,
        bool backed_in_path_only) {
      std::vector<
          std::pair<HypotheticalCandidate, fragment::FragmentTreeMembership>>
          response;
      response.reserve(candidates.size());
      std::transform(candidates.begin(),
                     candidates.end(),
                     std::back_inserter(response),
                     [](const HypotheticalCandidate &candidate)
                         -> std::pair<HypotheticalCandidate,
                                      fragment::FragmentTreeMembership> {
                       return {candidate, {}};
                     });

      const auto &required_active_leaf = fragment_tree_relay_parent;
      for (const auto &[active_leaf, leaf_view] : view.active_leaves) {
        if (required_active_leaf
            && required_active_leaf->get() != active_leaf) {
          continue;
        }

        for (auto &[c, membership] : response) {
          const ParachainId &para_id = candidatePara(c);
          auto it_fragment_tree = leaf_view.fragment_trees.find(para_id);
          if (it_fragment_tree == leaf_view.fragment_trees.end()) {
            continue;
          }

          auto it_candidate_storage = view.candidate_storage.find(para_id);
          if (it_candidate_storage == view.candidate_storage.end()) {
            continue;
          }

          const auto &fragment_tree = it_fragment_tree->second;
          const auto &candidate_storage = it_candidate_storage->second;
          const auto &candidate_hash = candidateHash(c);
          const auto &hypothetical = c;

          std::vector<size_t> depths =
              fragment_tree.hypotheticalDepths(candidate_hash,
                                               hypothetical,
                                               candidate_storage,
                                               backed_in_path_only);

          if (!depths.empty()) {
            membership.emplace_back(active_leaf, std::move(depths));
          }
        }
      }
      return response;
    }

    fragment::FragmentTreeMembership fragmentTreeMembership(
        const std::unordered_map<Hash, RelayBlockViewData> &active_leaves,
        ParachainId para,
        const CandidateHash &candidate) const {
      fragment::FragmentTreeMembership membership{};
      for (const auto &[relay_parent, view_data] : active_leaves) {
        if (auto it = view_data.fragment_trees.find(para);
            it != view_data.fragment_trees.end()) {
          const auto &tree = it->second;
          if (auto depths = tree.candidate(candidate)) {
            membership.emplace_back(relay_parent, *depths);
          }
        }
      }
      return membership;
    }

    void candidateSeconded(ParachainId para,
                           const CandidateHash &candidate_hash) {
      auto it = view.candidate_storage.find(para);
      if (it == view.candidate_storage.end()) {
        SL_WARN(logger,
                "Received instruction to second unknown candidate. (para "
                "id={}, candidate hash={})",
                para,
                candidate_hash);
        return;
      }

      auto &storage = it->second;
      if (!storage.contains(candidate_hash)) {
        SL_WARN(logger,
                "Received instruction to second unknown candidate in storage. "
                "(para "
                "id={}, candidate hash={})",
                para,
                candidate_hash);
        return;
      }

      storage.markSeconded(candidate_hash);
    }

    void candidateBacked(ParachainId para,
                         const CandidateHash &candidate_hash) {
      auto storage = view.candidate_storage.find(para);
      if (storage == view.candidate_storage.end()) {
        SL_WARN(logger,
                "Received instruction to back unknown candidate. (para_id={}, "
                "candidate_hash={})",
                para,
                candidate_hash);
        return;
      }
      if (!storage->second.contains(candidate_hash)) {
        SL_WARN(logger,
                "Received instruction to back unknown candidate. (para_id={}, "
                "candidate_hash={})",
                para,
                candidate_hash);
        return;
      }
      if (storage->second.isBacked(candidate_hash)) {
        SL_DEBUG(logger,
                 "Received redundant instruction to mark candidate as backed. "
                 "(para_id={}, candidate_hash={})",
                 para,
                 candidate_hash);
        return;
      }
      storage->second.markBacked(candidate_hash);
    }

    fragment::FragmentTreeMembership introduceCandidate(
        ParachainId para,
        const network::CommittedCandidateReceipt &candidate,
        const crypto::Hashed<const runtime::PersistedValidationData &, 32> &pvd,
        const CandidateHash &candidate_hash) {
      auto it_storage = view.candidate_storage.find(para);
      if (it_storage == view.candidate_storage.end()) {
        SL_WARN(logger,
                "Received seconded candidate for inactive para. (parachain "
                "id={}, candidate hash={})",
                para,
                candidate_hash);
        return {};
      }

      auto &storage = it_storage->second;
      if (auto res =
              storage.addCandidate(candidate_hash, candidate, pvd, hasher_);
          res.has_error()) {
        if (res.error()
            == fragment::CandidateStorage::Error::CANDIDATE_ALREADY_KNOWN) {
          return fragmentTreeMembership(
              view.active_leaves, para, candidate_hash);
        }
        if (res.error()
            == fragment::CandidateStorage::Error::
                PERSISTED_VALIDATION_DATA_MISMATCH) {
          SL_WARN(logger,
                  "Received seconded candidate had mismatching validation "
                  "data. (parachain id={}, candidate hash={})",
                  para,
                  candidate_hash);
          return {};
        }
      }

      fragment::FragmentTreeMembership membership{};
      for (auto &[relay_parent, leaf_data] : view.active_leaves) {
        if (auto it = leaf_data.fragment_trees.find(para);
            it != leaf_data.fragment_trees.end()) {
          auto &tree = it->second;
          tree.addAndPopulate(candidate_hash, storage);
          if (auto depths = tree.candidate(candidate_hash)) {
            membership.emplace_back(relay_parent, *depths);
          }
        }
      }

      if (membership.empty()) {
        storage.removeCandidate(candidate_hash, hasher_);
      }

      return membership;
    }
  };

}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_PROSPECTIVE_PARACHAINS_HPP
