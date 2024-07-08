/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/babe_lottery_impl.hpp"

#include "consensus/babe/babe_config_repository.hpp"
#include "consensus/babe/impl/prepare_transcript.hpp"
#include "consensus/babe/impl/threshold_util.hpp"
#include "crypto/hasher.hpp"
#include "crypto/key_store/session_keys.hpp"
#include "crypto/vrf_provider.hpp"

namespace kagome::consensus::babe {
  using common::Buffer;
  namespace vrf_constants = crypto::constants::sr25519::vrf;

  BabeLotteryImpl::BabeLotteryImpl(
      std::shared_ptr<BabeConfigRepository> config_repo,
      std::shared_ptr<crypto::SessionKeys> session_keys,
      std::shared_ptr<crypto::VRFProvider> vrf_provider,
      std::shared_ptr<crypto::Hasher> hasher)
      : logger_(log::createLogger("BabeLottery", "babe_lottery")),
        config_repo_(std::move(config_repo)),
        session_keys_(std::move(session_keys)),
        vrf_provider_(std::move(vrf_provider)),
        hasher_(std::move(hasher)) {
    BOOST_ASSERT(config_repo_);
    BOOST_ASSERT(session_keys_);
    BOOST_ASSERT(vrf_provider_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(logger_);
    epoch_ = std::numeric_limits<EpochNumber>::max();
  }

  bool BabeLotteryImpl::changeEpoch(EpochNumber epoch,
                                    const primitives::BlockInfo &best_block) {
    epoch_ = epoch;

    auto config_res = config_repo_->config(best_block, epoch);
    [[unlikely]] if (config_res.has_error()) {
      SL_ERROR(logger_,
               "Can not get epoch: {}; Epoch was not changed",
               config_res.error());
      return false;
    }
    auto &config = *config_res.value();

    keypair_ = session_keys_->getBabeKeyPair(config.authorities);
    if (!keypair_) {
      return false;
    }

    randomness_ = config.randomness;

    auth_number_ = config.authorities.size();

    threshold_ = babe::calculateThreshold(
        config.leadership_rate, config.authorities, keypair_->second);

    allowed_slots_ = config.allowed_slots;

    SL_TRACE(logger_, "Epoch changed to epoch {}", epoch_);
    return keypair_.has_value();
  }

  EpochNumber BabeLotteryImpl::getEpoch() const {
    return epoch_;
  }

  std::optional<SlotLeadership> BabeLotteryImpl::getSlotLeadership(
      const primitives::BlockHash &block, SlotNumber slot) const {
    BOOST_ASSERT_MSG(epoch_ != std::numeric_limits<uint64_t>::max(),
                     "Epoch must be initialized before this point");
    BOOST_ASSERT_MSG(keypair_.has_value(), "Node must be active validator");

    primitives::Transcript transcript;
    prepareTranscript(transcript, randomness_, slot, epoch_);

    auto vrf_output =
        vrf_provider_->signTranscript(transcript, *keypair_->first, threshold_);

    if (vrf_output.has_value()) {
      // Primary slot leadership
      return SlotLeadership{
          .slot_type = SlotType::Primary,
          .authority_index = keypair_->second,
          .keypair = keypair_->first,
          .vrf_output = vrf_output.value(),
      };
    }

    if (allowed_slots_ == AllowedSlots::PrimaryOnly) {
      // Secondary is not allowed
      return std::nullopt;
    }

    // Index of secondary leader
    auto auth_index_of_leader = be_bytes_to_uint256(hasher_->blake2b_256(
                                    scale::encode(randomness_, slot).value()))
                              % auth_number_;

    if (keypair_->second != auth_index_of_leader) {
      // Author is not a secondary leader
      return std::nullopt;
    }

    if (allowed_slots_ == AllowedSlots::PrimaryAndSecondaryVRF) {
      primitives::Transcript transcript;
      prepareTranscript(transcript, randomness_, slot, epoch_);
      auto vrf_output =
          vrf_provider_->signTranscript(transcript, *keypair_->first);

      BOOST_ASSERT(vrf_output.has_value());

      // SecondaryVRF slot leadership
      return SlotLeadership{
          .slot_type = SlotType::SecondaryVRF,
          .authority_index = keypair_->second,
          .keypair = keypair_->first,
          .vrf_output = vrf_output.value(),
      };
    }

    // SecondaryPlain slot leadership
    return SlotLeadership{
        .slot_type = SlotType::SecondaryPlain,
        .authority_index = keypair_->second,
        .keypair = keypair_->first,
        .vrf_output = {},
    };
  }
}  // namespace kagome::consensus::babe
