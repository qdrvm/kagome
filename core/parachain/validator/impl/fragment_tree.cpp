/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/fragment_tree.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::parachain::fragment,
                            Constraints::Error,
                            e) {
  using E = kagome::parachain::fragment::Constraints::Error;
  switch (e) {
    case E::DISALLOWED_HRMP_WATERMARK:
      return "Disallowed HRMP watermark";
    case E::NO_SUCH_HRMP_CHANNEL:
      return "No such HRMP channel";
    case E::HRMP_BYTES_OVERFLOW:
      return "HRMP bytes overflow";
    case E::HRMP_MESSAGE_OVERFLOW:
      return "HRMP message overflow";
    case E::UMP_MESSAGE_OVERFLOW:
      return "UMP message overflow";
    case E::UMP_BYTES_OVERFLOW:
      return "UMP bytes overflow";
    case E::DMP_MESSAGE_UNDERFLOW:
      return "DMP message underflow";
    case E::APPLIED_NONEXISTENT_CODE_UPGRADE:
      return "Applied nonexistent code upgrade";
  }
  return "Unknown error";
}

OUTCOME_CPP_DEFINE_CATEGORY(kagome::parachain::fragment,
                            CandidateStorage::Error,
                            e) {
  using E = kagome::parachain::fragment::CandidateStorage::Error;
  switch (e) {
    case E::CANDIDATE_ALREADY_KNOWN:
      return "Candidate already known";
    case E::PERSISTED_VALIDATION_DATA_MISMATCH:
      return "Persisted validation data mismatch";
  }
  return "Unknown error";
}

OUTCOME_CPP_DEFINE_CATEGORY(kagome::parachain::fragment,
                            Scope::Error,
                            e) {
  using E = kagome::parachain::fragment::Scope::Error;
  switch (e) {
    case E::UNEXPECTED_ANCESTOR:
      return "Unexpected ancestor";
  }
  return "Unknown error";
}

namespace kagome::parachain::fragment {

  outcome::result<Scope> Scope::withAncestors(
		ParachainId para,
		const fragment::RelayChainBlockInfo &relay_parent,
		const Constraints &base_constraints,
		const Vec<PendingAvailability> &pending_availability,
		size_t max_depth,
		const Vec<fragment::RelayChainBlockInfo> &ancestors
	) {
    Map<BlockNumber, fragment::RelayChainBlockInfo> ancestors_map;
    HashMap<Hash, fragment::RelayChainBlockInfo> ancestors_by_hash;

			auto prev = relay_parent.number;
			for (const auto &ancestor : ancestors) {
				if (prev == 0) {
					return Scope::Error::UNEXPECTED_ANCESTOR;
				} else if (ancestor.number != prev - 1) {
					return Scope::Error::UNEXPECTED_ANCESTOR;
				} else if (prev == base_constraints.min_relay_parent_number) {
					break;
				} else {
					prev = ancestor.number;
					ancestors_by_hash.emplace(ancestor.hash, ancestor);
					ancestors_map.emplace(ancestor.number, ancestor);
				}
			}

		return Scope {
			para,
			relay_parent,
      ancestors_map,
			ancestors_by_hash,
			pending_availability,
			base_constraints,
			max_depth,
		};
	}

  outcome::result<void> CandidateStorage::addCandidate(
      const CandidateHash &candidate_hash,
      const network::CommittedCandidateReceipt &candidate,
      const crypto::Hashed<const runtime::PersistedValidationData &, 32>
          &persisted_validation_data,
      const std::shared_ptr<crypto::Hasher> &hasher) {
    if (by_candidate_hash.find(candidate_hash) != by_candidate_hash.end()) {
      return Error::CANDIDATE_ALREADY_KNOWN;
    }

    if (persisted_validation_data.getHash()
        != candidate.descriptor.persisted_data_hash) {
      return Error::PERSISTED_VALIDATION_DATA_MISMATCH;
    }

    const auto parent_head_hash =
        hasher->blake2b_256(persisted_validation_data.get().parent_head);
    const auto output_head_hash =
        hasher->blake2b_256(candidate.commitments.para_head);

    by_parent_head[parent_head_hash].insert(candidate_hash);
    by_output_head[output_head_hash].insert(candidate_hash);

    by_candidate_hash.insert(
        {candidate_hash,
         CandidateEntry{
             .candidate_hash = candidate_hash,
             .relay_parent = candidate.descriptor.relay_parent,
             .candidate = ProspectiveCandidate{
                 .commitments = candidate.commitments,
                 .collator = candidate.descriptor.collator_id,
                 .collator_signature = candidate.descriptor.signature,
                 .persisted_validation_data = persisted_validation_data.get(),
                 .pov_hash = candidate.descriptor.pov_hash,
                 .validation_code_hash =
                     candidate.descriptor.validation_code_hash},
             .state = CandidateState::Introduced,
                     }});
    return outcome::success();
  }

  outcome::result<Constraints> Constraints::applyModifications(
      const ConstraintModifications &modifications) const {
    Constraints new_constraint{*this};
    if (modifications.required_parent) {
      new_constraint.required_parent = *modifications.required_parent;
    }

    if (modifications.hrmp_watermark) {
      const auto &hrmp_watermark = *modifications.hrmp_watermark;
      const auto hrmp_watermark_val = fromHrmpWatermarkUpdate(hrmp_watermark);

      auto it_b = new_constraint.hrmp_inbound.valid_watermarks.begin();
      auto it_e = new_constraint.hrmp_inbound.valid_watermarks.end();
      if (auto it = std::lower_bound(it_b, it_e, hrmp_watermark_val);
          !(it == it_e) && !(hrmp_watermark_val < *it)) {
        BOOST_ASSERT(it != it_e);
        new_constraint.hrmp_inbound.valid_watermarks.erase(it_b, it + 1);
      } else {
        const bool process = visit_in_place(
            hrmp_watermark,
            [&](const HrmpWatermarkUpdateHead &) {
              new_constraint.hrmp_inbound.valid_watermarks.erase(it_b, it);
              return true;
            },
            [&](const HrmpWatermarkUpdateTrunk &val) { return false; });
        if (!process) {
          return Error::DISALLOWED_HRMP_WATERMARK;
        }
      }
    }

    for (const auto &[id, outbound_hrmp_mod] : modifications.outbound_hrmp) {
      if (auto it = new_constraint.hrmp_channels_out.find(id);
          it != new_constraint.hrmp_channels_out.end()) {
        auto &outbound = it->second;
        OUTCOME_TRY(math::checked_sub(outbound.bytes_remaining,
                                      outbound_hrmp_mod.bytes_submitted,
                                      Error::HRMP_BYTES_OVERFLOW));
        OUTCOME_TRY(math::checked_sub(outbound.messages_remaining,
                                      outbound_hrmp_mod.messages_submitted,
                                      Error::HRMP_MESSAGE_OVERFLOW));
      } else {
        return Error::NO_SUCH_HRMP_CHANNEL;
      }
    }

    OUTCOME_TRY(math::checked_sub(new_constraint.ump_remaining,
                                  modifications.ump_messages_sent,
                                  Error::UMP_MESSAGE_OVERFLOW));
    OUTCOME_TRY(math::checked_sub(new_constraint.ump_remaining_bytes,
                                  modifications.ump_bytes_sent,
                                  Error::UMP_BYTES_OVERFLOW));

    if (modifications.dmp_messages_processed
        > new_constraint.dmp_remaining_messages.size()) {
      return Error::DMP_MESSAGE_UNDERFLOW;
    } else {
      new_constraint.dmp_remaining_messages.erase(
          new_constraint.dmp_remaining_messages.begin(),
          new_constraint.dmp_remaining_messages.begin()
              + modifications.dmp_messages_processed);
    }

    if (modifications.code_upgrade_applied) {
      if (auto new_code = std::move(new_constraint.future_validation_code)) {
        BOOST_ASSERT(!new_constraint.future_validation_code);
        new_constraint.validation_code_hash = std::move(new_code->second);
      } else {
        return Error::APPLIED_NONEXISTENT_CODE_UPGRADE;
      }
    }

    return new_constraint;
  }
}  // namespace kagome::parachain::fragment
