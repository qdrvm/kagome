/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/babe_lottery_impl.hpp"

#include <unordered_set>

#include <boost/assert.hpp>
#include "common/buffer.hpp"
#include "common/mp_utils.hpp"

namespace kagome::consensus {
  using common::Buffer;
  namespace vrf_constants = crypto::constants::sr25519::vrf;

  BabeLotteryImpl::BabeLotteryImpl(
      std::shared_ptr<crypto::VRFProvider> vrf_provider,
      std::shared_ptr<crypto::Hasher> hasher)
      : vrf_provider_{std::move(vrf_provider)}, hasher_{std::move(hasher)} {
    BOOST_ASSERT(vrf_provider_);
    BOOST_ASSERT(hasher_);
  }

  BabeLottery::SlotsLeadership BabeLotteryImpl::slotsLeadership(
      const Epoch &epoch, crypto::SR25519Keypair keypair) const {
    std::vector<boost::optional<crypto::VRFOutput>> result;
    result.reserve(epoch.epoch_duration);

    // randomness || slot number
    Buffer vrf_input(vrf_constants::OUTPUT_SIZE + 8, 0);

    // the first part - randomness - is always the same, while the slot number
    // obviously changes depending on the slot we are computing for
    std::copy(
        epoch.randomness.begin(), epoch.randomness.end(), vrf_input.begin());

    auto slot_number_begin = vrf_input.begin() + vrf_constants::OUTPUT_SIZE;
    for (BabeSlotNumber i = 0; i < epoch.epoch_duration; ++i) {
      auto slot_bytes = common::uint64_t_to_bytes(i);
      std::copy(slot_bytes.begin(), slot_bytes.end(), slot_number_begin);
      result.push_back(
          vrf_provider_->sign(vrf_input, keypair, epoch.threshold));
    }

    return result;
  }

  Randomness BabeLotteryImpl::computeRandomness(
      Randomness last_epoch_randomness, EpochIndex last_epoch_index) {
    static std::unordered_set<EpochIndex> computed_epochs_randomnesses{};

    // the function must never be called twice for the same epoch
    BOOST_ASSERT(computed_epochs_randomnesses.insert(last_epoch_index).second);

    // randomness || epoch_index || rho
    Buffer new_randomness(
        vrf_constants::OUTPUT_SIZE + 8 + last_epoch_vrf_values_.size() * 32, 0);

    std::copy(last_epoch_randomness.begin(),
              last_epoch_randomness.end(),
              new_randomness.begin());

    auto epoch_index_bytes = common::uint64_t_to_bytes(last_epoch_index);
    std::copy(epoch_index_bytes.begin(),
              epoch_index_bytes.end(),
              new_randomness.begin() + vrf_constants::OUTPUT_SIZE);

    auto new_vrf_value_begin =
        new_randomness.begin() + vrf_constants::OUTPUT_SIZE + 8;
    // NOLINTNEXTLINE
    for (size_t i = 0; i < last_epoch_vrf_values_.size(); ++i) {
      auto value_bytes = common::uint256_t_to_bytes(last_epoch_vrf_values_[i]);
      std::copy(value_bytes.begin(), value_bytes.end(), new_vrf_value_begin);
      new_vrf_value_begin += 32;
    }
    last_epoch_vrf_values_.clear();

    return hasher_->blake2b_256(new_randomness);
  }

  void BabeLotteryImpl::submitVRFValue(const crypto::VRFValue &value) {
    last_epoch_vrf_values_.push_back(value);
  }
}  // namespace kagome::consensus
