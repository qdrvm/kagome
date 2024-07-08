/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/availability/bitfield/store_impl.hpp"

namespace kagome::parachain {
  BitfieldStoreImpl::BitfieldStoreImpl(
      std::shared_ptr<runtime::ParachainHost> parachain_api)
      : parachain_api_(std::move(parachain_api)) {
    BOOST_ASSERT(parachain_api_);
  }

  void BitfieldStoreImpl::putBitfield(const BlockHash &relay_parent,
                                      const SignedBitfield &bitfield) {
    SL_TRACE(logger_,
             "Put bitfields.(relay_parent={}, bitfields={})",
             relay_parent,
             bitfield);
    bitfields_[relay_parent].push_back(bitfield);
  }

  void BitfieldStoreImpl::remove(const BlockHash &relay_parent) {
    bitfields_.erase(relay_parent);
  }

  void BitfieldStoreImpl::printStoragesLoad() {
    SL_TRACE(logger_,
             "[Bitfield store statistics]:"
             "\n\t-> bitfields_={}",
             bitfields_.size());
  }

  std::vector<BitfieldStore::SignedBitfield> BitfieldStoreImpl::getBitfields(
      const BlockHash &relay_parent) const {
    auto it = bitfields_.find(relay_parent);
    if (it == bitfields_.end()) {
      return {};
    }

    auto c = parachain_api_->availability_cores(relay_parent);
    if (c.has_error()) {
      logger_->warn(
          "Availability cores not present.(relay parent={}, error={})",
          relay_parent,
          c.error());
      return {};
    }
    auto &cores = c.value();

    std::map<ValidatorIndex, SignedBitfield> selected;
    for (const auto &bf : it->second) {
      if (bf.payload.payload.bits.size() != cores.size()) {
        logger_->warn(
            "dropping bitfield due to length mismatch.(relay parent={})",
            relay_parent);
        continue;
      }

      auto it_selected = selected.find(bf.payload.ix);
      if (it_selected == selected.end()
          || approval::count_ones(it_selected->second.payload.payload)
                 < approval::count_ones(bf.payload.payload)) {
        bool skip = false;
        for (size_t ix = 0; ix < cores.size(); ++ix) {
          auto &core = cores[ix];
          if (std::holds_alternative<runtime::OccupiedCore>(core)) {
            continue;
          }

          if (bf.payload.payload.bits[ix]) {
            SL_INFO(logger_,
                    "dropping invalid bitfield - bit is set for an unoccupied "
                    "core.(relay_parent={})",
                    relay_parent);
            skip = true;
            break;
          }
        }

        if (!skip) {
          selected[bf.payload.ix] = bf;
        }
      }
    }

    std::vector<SignedBitfield> bitfields;
    bitfields.reserve(selected.size());
    for (auto &bf : selected) {
      bitfields.push_back(bf.second);
    }
    return bitfields;
  }
}  // namespace kagome::parachain
