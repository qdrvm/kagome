/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/prospective_parachains/common.hpp"
#include "utils/stringify.hpp"

#define COMPONENT Constraints
#define COMPONENT_NAME STRINGIFY(COMPONENT)

OUTCOME_CPP_DEFINE_CATEGORY(kagome::parachain::fragment,
                            Constraints::Error,
                            e) {
  using E = kagome::parachain::fragment::Constraints::Error;
  switch (e) {
    case E::DISALLOWED_HRMP_WATERMARK:
      return COMPONENT_NAME ": Disallowed HRMP watermark";
    case E::NO_SUCH_HRMP_CHANNEL:
      return COMPONENT_NAME ": No such HRMP channel";
    case E::HRMP_BYTES_OVERFLOW:
      return COMPONENT_NAME ": HRMP bytes overflow";
    case E::HRMP_MESSAGE_OVERFLOW:
      return COMPONENT_NAME ": HRMP message overflow";
    case E::UMP_MESSAGE_OVERFLOW:
      return COMPONENT_NAME ": UMP message overflow";
    case E::UMP_BYTES_OVERFLOW:
      return COMPONENT_NAME ": UMP bytes overflow";
    case E::DMP_MESSAGE_UNDERFLOW:
      return COMPONENT_NAME ": DMP message underflow";
    case E::APPLIED_NONEXISTENT_CODE_UPGRADE:
      return COMPONENT_NAME ": Applied nonexistent code upgrade";
  }
  return COMPONENT_NAME ": Unknown error";
}

namespace kagome::parachain::fragment {

  outcome::result<void> Constraints::check_modifications(
      const ConstraintModifications &modifications) const {
    if (modifications.hrmp_watermark) {
      if (auto hrmp_watermark = if_type<const HrmpWatermarkUpdateTrunk>(
              *modifications.hrmp_watermark)) {
        bool found = false;
        for (const BlockNumber &w : hrmp_inbound.valid_watermarks) {
          if (w == hrmp_watermark->get().v) {
            found = true;
            break;
          }
        }
        if (!found) {
          return Error::DISALLOWED_HRMP_WATERMARK;
        }
      }
    }

    /// TODO(iceseer): do
    /// implement
    return outcome::success();
  }

  outcome::result<Constraints> Constraints::apply_modifications(
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

}
