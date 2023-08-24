/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/babe_lottery_impl.hpp"

#include "consensus/validation/prepare_transcript.hpp"
#include "crypto/hasher.hpp"
#include "crypto/vrf_provider.hpp"

namespace kagome::consensus::babe {
  using common::Buffer;
  namespace vrf_constants = crypto::constants::sr25519::vrf;

  BabeLotteryImpl::BabeLotteryImpl(
      std::shared_ptr<crypto::VRFProvider> vrf_provider,
      std::shared_ptr<BabeConfigRepository> babe_config_repo,
      std::shared_ptr<crypto::Hasher> hasher)
      : vrf_provider_{std::move(vrf_provider)},
        hasher_{std::move(hasher)},
        logger_{log::createLogger("BabeLottery", "babe_lottery")} {
    BOOST_ASSERT(vrf_provider_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(logger_);
    BOOST_ASSERT(babe_config_repo);
    epoch_.epoch_number = std::numeric_limits<uint64_t>::max();
  }

  void BabeLotteryImpl::changeEpoch(const EpochDescriptor &epoch,
                                    const Randomness &randomness,
                                    const Threshold &threshold,
                                    const crypto::Sr25519Keypair &keypair) {
    SL_TRACE(logger_,
             "Epoch changed "
             "FROM epoch {} with randomness {} TO epoch {} with randomness {}",
             epoch_.epoch_number,
             randomness_,
             epoch.epoch_number,
             randomness);

    epoch_ = epoch;
    randomness_ = randomness;
    threshold_ = threshold;
    keypair_ = keypair;
  }

  EpochDescriptor BabeLotteryImpl::getEpoch() const {
    return epoch_;
  }

  std::optional<crypto::VRFOutput> BabeLotteryImpl::getSlotLeadership(
      SlotNumber slot) const {
    BOOST_ASSERT_MSG(
        epoch_.epoch_number != std::numeric_limits<uint64_t>::max(),
        "Epoch must be initialized before this point");

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

  crypto::VRFOutput BabeLotteryImpl::slotVrfSignature(SlotNumber slot) const {
    BOOST_ASSERT_MSG(
        epoch_.epoch_number != std::numeric_limits<uint64_t>::max(),
        "Epoch must be initialized before this point");

    primitives::Transcript transcript;
    prepareTranscript(transcript, randomness_, slot, epoch_.epoch_number);
    auto res = vrf_provider_->signTranscript(transcript, keypair_);

    BOOST_ASSERT(res);
    return res.value();
  }

  std::optional<primitives::AuthorityIndex>
  BabeLotteryImpl::secondarySlotAuthor(
      SlotNumber slot,
      primitives::AuthorityListSize authorities_count,
      const Randomness &randomness) const {
    if (0 == authorities_count) {
      return std::nullopt;
    }

    auto rand = hasher_->blake2b_256(
        scale::encode(std::tuple(randomness, slot)).value());

    auto rand_number = common::be_bytes_to_uint256(rand);

    auto index = (rand_number % authorities_count)
                     .convert_to<primitives::AuthorityIndex>();

    SL_TRACE(logger_,
             "Secondary author for slot {} has index {}. "
             "({} authorities. Randomness: {})",
             slot,
             index,
             authorities_count,
             randomness);
    return index;
  }

}  // namespace kagome::consensus::babe
