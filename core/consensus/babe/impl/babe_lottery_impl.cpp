/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/babe_lottery_impl.hpp"

#include <unordered_set>

#include <boost/assert.hpp>
#include "common/buffer.hpp"
#include "common/mp_utils.hpp"
#include "consensus/validation/prepare_transcript.hpp"

namespace kagome::consensus {
  using common::Buffer;
  namespace vrf_constants = crypto::constants::sr25519::vrf;

  BabeLotteryImpl::BabeLotteryImpl(
      std::shared_ptr<crypto::VRFProvider> vrf_provider,
      std::shared_ptr<primitives::BabeConfiguration> configuration,
      std::shared_ptr<crypto::Hasher> hasher)
      : vrf_provider_{std::move(vrf_provider)},
        hasher_{std::move(hasher)},
        logger_{log::createLogger("BabeLottery", "babe_lottery")} {
    BOOST_ASSERT(vrf_provider_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(logger_);
    BOOST_ASSERT(configuration);
    epoch_length_ = configuration->epoch_length;
    epoch_.epoch_number = std::numeric_limits<uint64_t>::max();
  }

  void BabeLotteryImpl::changeEpoch(const EpochDescriptor &epoch,
                                    const Randomness &randomness,
                                    const Threshold &threshold,
                                    const crypto::Sr25519Keypair &keypair) {
    epoch_ = epoch;
    randomness_ = randomness;
    threshold_ = threshold;
    keypair_ = keypair;
  }

  EpochDescriptor BabeLotteryImpl::getEpoch() const {
    return epoch_;
  }

  std::optional<crypto::VRFOutput> BabeLotteryImpl::getSlotLeadership(
      primitives::BabeSlotNumber slot) const {
    primitives::Transcript transcript;
    prepareTranscript(transcript, randomness_, slot, epoch_.epoch_number);

    auto res = vrf_provider_->signTranscript(transcript, keypair_, threshold_);

    SL_TRACE(
        logger_,
        "prepareTranscript (leadership): randomness {}, slot {}, epoch {}{}",
        randomness_,
        slot,
        epoch_.epoch_number,
        res.has_value() ? " - SLOT LEADER" : "");

    return res;
  }

  crypto::VRFOutput BabeLotteryImpl::slotVrfSignature(
      primitives::BabeSlotNumber slot) const {
    primitives::Transcript transcript;
    prepareTranscript(transcript, randomness_, slot, epoch_.epoch_number);
    auto res = vrf_provider_->signTranscript(transcript, keypair_);

    BOOST_ASSERT(res);
    return res.value();
  }

  Randomness BabeLotteryImpl::computeRandomness(
      const Randomness &last_epoch_randomness, EpochNumber last_epoch_number) {
    static std::unordered_set<EpochNumber> computed_epochs_randomnesses{};

    // the function must never be called twice for the same epoch
    BOOST_ASSERT(computed_epochs_randomnesses.insert(last_epoch_number).second);

    // randomness || epoch_number || rho
    Buffer new_randomness(
        vrf_constants::OUTPUT_SIZE + 8 + last_epoch_vrf_values_.size() * 32, 0);

    std::copy(last_epoch_randomness.begin(),
              last_epoch_randomness.end(),
              new_randomness.begin());

    auto epoch_number_bytes = common::uint64_t_to_bytes(last_epoch_number);
    std::copy(epoch_number_bytes.begin(),
              epoch_number_bytes.end(),
              new_randomness.begin() + vrf_constants::OUTPUT_SIZE);

    auto new_vrf_value_begin =
        new_randomness.begin() + vrf_constants::OUTPUT_SIZE + 8;
    // NOLINTNEXTLINE
    for (size_t i = 0; i < last_epoch_vrf_values_.size(); ++i) {
      auto const &value_bytes = last_epoch_vrf_values_[i];
      std::copy(value_bytes.begin(), value_bytes.end(), new_vrf_value_begin);
      new_vrf_value_begin += 32;
    }
    last_epoch_vrf_values_.clear();

    return hasher_->blake2b_256(new_randomness);
  }

  void BabeLotteryImpl::submitVRFValue(const crypto::VRFPreOutput &value) {
    last_epoch_vrf_values_.push_back(value);
  }

  std::optional<primitives::AuthorityIndex>
  BabeLotteryImpl::secondarySlotAuthor(
      primitives::BabeSlotNumber slot,
      primitives::AuthorityListSize authorities_count,
      const Randomness &randomness) const {
    if (0 == authorities_count) {
      return std::nullopt;
    }

    auto slot_bytes = common::uint64_t_to_bytes(slot);
    common::Buffer seed;
    seed.put(randomness);
    seed.put(slot_bytes);
    auto rand = hasher_->blake2b_256(seed);

    auto rand_number = common::bytes_to_uint256_t(rand);
    auto index = static_cast<primitives::AuthorityIndex>(rand_number
                                                         % authorities_count);
    SL_TRACE(logger_,
             "Secondary slot author for slot {}, authorities count {}, "
             "randomness {} is {}",
             slot,
             authorities_count,
             randomness.toHex(),
             index);
    return index;
  }

}  // namespace kagome::consensus
