/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/approval/approval-distribution.hpp"
#include "common/visitor.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "crypto/crypto_store.hpp"
#include "parachain/approval/approval.hpp"
#include "primitives/authority.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"
#include "utils/async_sequence.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::parachain, ApprovalDistribution::Error, e) {
  using E = kagome::parachain::ApprovalDistribution::Error;
  switch (e) {
    case E::NO_INSTANCE:
      return "No self instance";
    case E::NO_CONTEXT:
      return "No context";
    case E::NO_SESSION_INFO:
      return "No session info";
    case E::UNUSED_SLOT_TYPE:
      return "Unused slot type";
  }
  return "Unknown approval-distribution error";
}

static constexpr uint64_t TICK_DURATION_MILLIS = 500;

namespace {

  /// assumes `slot_duration_millis` evenly divided by tick duration.
  kagome::network::Tick slot_number_to_tick(
      uint64_t slot_duration_millis, kagome::consensus::babe::BabeSlotNumber slot) {
    auto ticks_per_slot = slot_duration_millis / TICK_DURATION_MILLIS;
    return slot * ticks_per_slot;
  }

  bool is_in_backing_group(
      std::vector<std::vector<kagome::parachain::ValidatorIndex>> const
          &validator_groups,
      kagome::parachain::ValidatorIndex validator,
      kagome::parachain::GroupIndex group) {
    return std::find_if(
               validator_groups.begin(),
               validator_groups.end(),
               [validator](auto const &group) {
                 return std::count(group.begin(), group.end(), validator) > 0;
               })
           != validator_groups.end();
  }

  void compute_vrf_modulo_assignments(
      const kagome::common::Buffer &keypair_buf,
      const kagome::runtime::SessionInfo &config,
      const RelayVRFStory &relay_vrf_story,
      const std::vector<kagome::parachain::CoreIndex> &lc,
      kagome::parachain::ValidatorIndex validator_ix,
      std::unordered_map<kagome::parachain::CoreIndex,
                         kagome::parachain::ApprovalDistribution::OurAssignment>
          &assignments) {
    using namespace kagome::parachain;
    using namespace kagome;

    VRFCOutput cert_output;
    VRFCProof cert_proof;
    CoreIndex core{};
    for (uint32_t rvm_sample = 0; rvm_sample < config.relay_vrf_modulo_samples;
         ++rvm_sample) {
      if (sr25519_relay_vrf_modulo_assignments_cert(keypair_buf.data(),
                                                    rvm_sample,
                                                    config.n_cores,
                                                    &relay_vrf_story,
                                                    lc.data(),
                                                    lc.size(),
                                                    &cert_output,
                                                    &cert_proof,
                                                    &core)) {
        if (assignments.count(core) > 0) {
          continue;
        }

        crypto::VRFPreOutput o;
        std::copy_n(std::make_move_iterator(cert_output.data),
                    crypto::constants::sr25519::vrf::OUTPUT_SIZE,
                    o.begin());

        crypto::VRFProof p;
        std::copy_n(std::make_move_iterator(cert_proof.data),
                    crypto::constants::sr25519::vrf::PROOF_SIZE,
                    p.begin());

        assignments.emplace(
            core,
            ApprovalDistribution::OurAssignment{
                .cert =
                    approval::AssignmentCert{
                        .kind = approval::RelayVRFModulo{.sample = rvm_sample},
                        .vrf = crypto::VRFOutput{.output = o, .proof = p}},
                .tranche = 0ul,
                .validator_index = validator_ix,
                .triggered = false});
      }
    }
  }

  void compute_vrf_delay_assignments(
      const kagome::common::Buffer &keypair_buf,
      const kagome::runtime::SessionInfo &config,
      const RelayVRFStory &relay_vrf_story,
      const std::vector<kagome::parachain::CoreIndex> &lc,
      kagome::parachain::ValidatorIndex validator_ix,
      std::unordered_map<kagome::parachain::CoreIndex,
                         kagome::parachain::ApprovalDistribution::OurAssignment>
          &assignments) {
    using namespace kagome::parachain;
    using namespace kagome;

    VRFCOutput cert_output;
    VRFCProof cert_proof;
    uint32_t tranche = {};
    for (auto const &core : lc) {
      sr25519_relay_vrf_delay_assignments_cert(
          keypair_buf.data(),
          config.n_delay_tranches,
          config.zeroth_delay_tranche_width,
          &relay_vrf_story,
          core,
          &cert_output,
          &cert_proof,
          &tranche);

      auto it = assignments.find(core);
      if (it == assignments.end() || it->second.tranche > tranche) {
        crypto::VRFPreOutput o;
        std::copy_n(std::make_move_iterator(cert_output.data),
                    crypto::constants::sr25519::vrf::OUTPUT_SIZE,
                    o.begin());

        crypto::VRFProof p;
        std::copy_n(std::make_move_iterator(cert_proof.data),
                    crypto::constants::sr25519::vrf::PROOF_SIZE,
                    p.begin());

        assignments.insert_or_assign(
            core,
            ApprovalDistribution::OurAssignment{
                .cert =
                    approval::AssignmentCert{
                        .kind = approval::RelayVRFDelay{.core_index = core},
                        .vrf = crypto::VRFOutput{.output = std::move(o),
                                                 .proof = std::move(p)}},
                .tranche = tranche,
                .validator_index = validator_ix,
                .triggered = false});
      }
    }
  }

}  // namespace

namespace kagome::parachain {

  ApprovalDistribution::ApprovalDistribution(
      std::shared_ptr<ThreadPool> thread_pool,
      std::shared_ptr<runtime::ParachainHost> parachain_host,
      std::shared_ptr<consensus::babe::BabeUtil> babe_util,
      std::shared_ptr<crypto::CryptoStore> keystore,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<const blockchain::BlockTree> block_tree,
      std::shared_ptr<boost::asio::io_context> this_context,
      const ApprovalVotingSubsystem &config,
      std::shared_ptr<network::PeerView> peer_view,
      std::unique_ptr<clock::Timer> timer)
      : thread_pool_{std::move(thread_pool)},
        parachain_host_(std::move(parachain_host)),
        babe_util_(std::move(babe_util)),
        keystore_(std::move(keystore)),
        hasher_(std::move(hasher)),
        block_tree_(std::move(block_tree)),
        this_context_(std::move(this_context)),
        config_(std::move(config)),
        peer_view_(std::move(peer_view)),
        timer_{std::move(timer)} {
    BOOST_ASSERT(thread_pool_);
    BOOST_ASSERT(parachain_host_);
    BOOST_ASSERT(babe_util_);
    BOOST_ASSERT(keystore_);
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(this_context_);
    BOOST_ASSERT(peer_view_);
    BOOST_ASSERT(timer_);
  }

  ApprovalDistribution::~ApprovalDistribution() {
    BOOST_ASSERT(nullptr == int_pool_);
  }

  bool ApprovalDistribution::prepare() {
    BOOST_ASSERT(nullptr == int_pool_);
    int_pool_ = std::make_shared<ThreadPool>(1ull);
    my_view_sub_ = std::make_shared<network::PeerView::MyViewSubscriber>(
        peer_view_->getMyViewObservable(), false);
    my_view_sub_->subscribe(my_view_sub_->generateSubscriptionSetId(),
                            network::PeerView::EventType::kViewUpdated);
    my_view_sub_->setCallback([wptr{weak_from_this()}](
                                  auto /*set_id*/,
                                  auto && /*internal_obj*/,
                                  auto /*event_type*/,
                                  const network::View &event) {
      if (auto self = wptr.lock())
        self->on_active_leaves_update(event.finalized_number_, event.heads_);
    });

    return true;
  }

  bool ApprovalDistribution::start() {
    return true;
  }

  void ApprovalDistribution::stop() {
    int_pool_.reset();
  }

  std::optional<std::pair<ValidatorIndex, crypto::Sr25519Keypair>>
  ApprovalDistribution::findAssignmentKey(
      std::shared_ptr<crypto::CryptoStore> const &keystore,
      runtime::SessionInfo const &config) {
    for (size_t ix = 0; ix < config.assignment_keys.size(); ++ix) {
      auto const &pk = config.assignment_keys[ix];
      if (auto res = keystore->findSr25519Keypair(
              crypto::KEY_TYPE_ASGN,
              crypto::Sr25519PublicKey::fromSpan(pk).value());
          res.has_value()) {
        return std::make_pair((ValidatorIndex)ix, std::move(res.value()));
      }
    }
    SL_TRACE(logger_, "No assignment key");
    return std::nullopt;
  }

  ApprovalDistribution::AssignmentsList
  ApprovalDistribution::compute_assignments(
      std::shared_ptr<crypto::CryptoStore> const &keystore,
      runtime::SessionInfo const &config,
      RelayVRFStory const &relay_vrf_story,
      CandidateIncludedList const &leaving_cores) {
    if (config.n_cores == 0 || config.assignment_keys.empty()
        || config.validator_groups.empty()) {
      SL_DEBUG(logger_,
               "Not producing assignments because config is degenerate "
               "(n_cores:{}, has_assignment_keys:{}, has_validator_groups:{})",
               config.n_cores,
               config.assignment_keys.size(),
               config.validator_groups.size());
      return {};
    }

    std::optional<std::pair<ValidatorIndex, crypto::Sr25519Keypair>>
        founded_key = findAssignmentKey(keystore, config);
    if (!founded_key) {
      return {};
    }

    auto const &[validator_ix, assignments_key] = *founded_key;
    std::vector<CoreIndex> lc;
    for (auto const &[candidate_hash, _, core_ix, group_ix] : leaving_cores) {
      if (is_in_backing_group(config.validator_groups, validator_ix, group_ix))
        continue;
      lc.push_back(core_ix);
    }

    common::Buffer keypair_buf{};
    keypair_buf.put(assignments_key.secret_key).put(assignments_key.public_key);

    std::unordered_map<CoreIndex, ApprovalDistribution::OurAssignment>
        assignments;
    compute_vrf_modulo_assignments(
        keypair_buf, config, relay_vrf_story, lc, validator_ix, assignments);
    compute_vrf_delay_assignments(
        keypair_buf, config, relay_vrf_story, lc, validator_ix, assignments);

    return assignments;
  }

  void ApprovalDistribution::imported_block_info(
      const primitives::BlockHash &block_hash,
      const primitives::BlockHeader &block_header,
      const std::shared_ptr<boost::asio::io_context> &callback_exec_context,
      std::function<void(outcome::result<ImportedBlockInfo> &&)> &&callback) {
    if (approving_context_map_.count(block_hash) == 0ull) {
      approving_context_map_.emplace(
          block_hash,
          ApprovingContext{.block_header = block_header,
                           .included_candidates = std::nullopt,
                           .session_index = std::nullopt,
                           .babe_block_header = std::nullopt,
                           .babe_epoch = std::nullopt,
                           .session_info = std::nullopt,
                           .complete_callback_context = callback_exec_context,
                           .complete_callback = std::move(callback)});
      request_included_candidates(block_hash);
      request_session_index_and_info(block_hash, block_header.parent_hash);
      request_babe_epoch_and_block_header(
          callback_exec_context, block_header, block_hash);
    }
  }

  template <typename Func>
  void ApprovalDistribution::for_ACU(const primitives::BlockHash &block_hash,
                                     Func &&func) {
    if (auto it = approving_context_map_.find(block_hash);
        it != approving_context_map_.end()) {
      std::forward<Func>(func)(*it);
    }
  }

  void ApprovalDistribution::store_included_candidates(
      ApprovingContextUnit &acu,
      ApprovalDistribution::CandidateIncludedList const &candidates_list) {
    ApprovingContext &context = acu.second;
    context.included_candidates = candidates_list;
  }

  void ApprovalDistribution::try_process_approving_context(
      ApprovalDistribution::ApprovingContextUnit &acu) {
    ApprovingContext &ac = acu.second;
    if (!ac.is_complete()) {
      return;
    }

    switch (ac.babe_block_header->slotType()) {
      case consensus::babe::SlotType::Primary:
      case consensus::babe::SlotType::SecondaryVRF:
        break;

      case consensus::babe::SlotType::SecondaryPlain:
      default:
        return;
    }

    approval::UnsafeVRFOutput unsafe_vrf{
        .vrf_output = ac.babe_block_header->vrf_output,
        .slot = ac.babe_block_header->slot_number,
        .authority_index = ac.babe_block_header->authority_index};

    /// TODO(iceseer): implement when DigestTracker will be ready.
    /// unsafe_vrf.compute_randomness()
    outcome::result<RelayVRFStory> relay_vrf = RelayVRFStory{};

    auto assignments = compute_assignments(keystore_,
                                           *ac.session_info,
                                           relay_vrf.value(),
                                           *ac.included_candidates);

    /// TODO(iceseer): force approve impl

    ImportedBlockInfo imported_block{
        .included_candidates = std::move(*ac.included_candidates),
        .session_index = *ac.session_index,
        .assignments = std::move(assignments),
        .n_validators = ac.session_info->validators.size(),
        .relay_vrf_story = relay_vrf.value(),
        .slot = unsafe_vrf.slot,
        .session_info = std::move(*ac.session_info),
        .force_approve = std::nullopt};

    sequenceIgnore(ac.complete_callback_context->wrap(
        asAsync([callback{std::move(ac.complete_callback)},
                 imported_block{std::move(
                     imported_block)}]() mutable -> outcome::result<void>{
          callback(std::move(imported_block));
          return outcome::success();
        })));
  }

  void ApprovalDistribution::request_session_index_and_info(
      const primitives::BlockHash &block_hash,
      const primitives::BlockHash &parent_hash) {
    sequenceIgnore(
        thread_pool_->io_context()->wrap(
            asAsync([parent_hash,
                     block_hash,
                     logger{logger_},
                     parachain_host{parachain_host_}]()
                        -> outcome::result<
                            std::pair<SessionIndex, runtime::SessionInfo>> {
              OUTCOME_TRY(session_index,
                          parachain_host->session_index_for_child(parent_hash));

              /// TODO(iceseer): тут мы должны скипать сессии меньше
              /// `earliest_session`

              /// TODO(iceseer): check that we use block_hash here and not
              /// parent_hash!!!
              OUTCOME_TRY(
                  session_info,
                  parachain_host->session_info(block_hash, session_index));

              if (!session_info) {
                SL_ERROR(
                    logger,
                    "No session info for [session_index: {}, block_hash: {}]",
                    session_index,
                    block_hash);
                return Error::NO_SESSION_INFO;
              }

              return std::make_pair(session_index, std::move(*session_info));
            })),
        int_pool_->io_context()->wrap(asAsync(
            [block_hash, wptr{weak_from_this()}](
                auto &&session_data) mutable -> outcome::result<void> {
              if (auto self = wptr.lock()) {
                self->template for_ACU(block_hash, [&](auto &acu) {
                  acu.second.session_index = session_data.first;
                  acu.second.session_info = std::move(session_data.second);
                  self->try_process_approving_context(acu);
                });
              }
              return outcome::success();
            })));
  }

  void ApprovalDistribution::request_babe_epoch_and_block_header(
      const std::shared_ptr<boost::asio::io_context> &exec_context,
      const primitives::BlockHeader &block_header,
      const primitives::BlockHash &block_hash) {
    sequenceIgnore(
        exec_context->wrap(
            asAsync([babe_util{babe_util_}, block_header]()
                        -> outcome::result<
                            std::pair<consensus::babe::EpochNumber,
                                      consensus::babe::BabeBlockHeader>> {
              OUTCOME_TRY(babe_digests,
                          consensus::babe::getBabeDigests(block_header));
              const consensus::babe::EpochNumber epoch_number =
                  babe_util->slotToEpoch(babe_digests.second.slot_number);
              return std::make_pair(epoch_number, babe_digests.second);
            })),
        int_pool_->io_context()->wrap(asAsync(
            [block_hash, wptr{weak_from_this()}](
                auto &&babe_block_data) mutable -> outcome::result<void> {
              if (auto self = wptr.lock()) {
                self->template for_ACU(block_hash, [&](auto &acu) {
                  acu.second.babe_epoch = babe_block_data.first;
                  acu.second.babe_block_header = babe_block_data.second;
                  self->try_process_approving_context(acu);
                });
              }
              return outcome::success();
            })));
  }

  void ApprovalDistribution::request_included_candidates(
      const primitives::BlockHash &block_hash) {
    sequenceIgnore(
        thread_pool_->io_context()->wrap(asAsync(
            [hasher{hasher_}, block_hash, parachain_host{parachain_host_}]() mutable
                -> outcome::result<
                    ApprovalDistribution::CandidateIncludedList> {
              OUTCOME_TRY(candidates,
                          parachain_host->candidate_events(block_hash));
              ApprovalDistribution::CandidateIncludedList included;

              for (auto &candidate : candidates) {
                if (auto obj = if_type<runtime::CandidateIncluded>(candidate)) {
                  included.emplace_back(std::make_tuple(
                      hasher->blake2b_256(
                          scale::encode(obj->get().candidate_receipt).value()),
                      std::move(obj->get().candidate_receipt),
                      obj->get().core_index,
                      obj->get().group_index));
                }
              }

              return included;
            })),
        int_pool_->io_context()->wrap(
            asAsync([block_hash, wptr{weak_from_this()}](
                        auto
                            &&candidates_included_list) mutable -> outcome::result<void> {
              if (auto self = wptr.lock()) {
                self->template for_ACU(block_hash, [&](auto &acu) {
                  self->store_included_candidates(
                      acu, std::move(candidates_included_list));
                  self->try_process_approving_context(acu);
                });
              }
              return outcome::success();
            })));
  }

  std::vector<std::pair<CandidateHash, ApprovalDistribution::CandidateEntry>>
  ApprovalDistribution::add_block_entry(primitives::BlockNumber block_number,
                                        const primitives::BlockHash &block_hash,
                                        ImportedBlockInfo &&block_info) {
    std::vector<std::pair<CandidateHash, CandidateEntry>> entries;
    if (auto blocks = storedBlocks().get_or_create(block_number);
        blocks.get().find(block_hash) == blocks.get().end()) {
      return entries;
    } else {
      blocks.get().insert(block_hash);
    }

    entries.reserve(block_info.included_candidates.size());
    for (auto const &[candidateHash, candidateReceipt, coreIndex, groupIndex] :
         block_info.included_candidates) {
      std::optional<std::reference_wrapper<OurAssignment>> assignment{};
      if (auto assignment_it = block_info.assignments.find(coreIndex);
          assignment_it != block_info.assignments.end()) {
        assignment = assignment_it->second;
      }

      auto candidate_entry =
          storedCandidateEntries().get_or_create(candidateHash,
                                                 candidateReceipt,
                                                 block_info.session_index,
                                                 block_info.n_validators);
      candidate_entry.get().block_assignments.insert_or_assign(
          block_hash,
          ApprovalEntry(groupIndex, assignment, block_info.n_validators));
      entries.emplace_back(candidateHash, candidate_entry.get());
    }
    return entries;
  }

  ApprovalDistribution::BlockImportedCandidates
  ApprovalDistribution::processImportedBlock(
      primitives::BlockNumber block_number,
      const primitives::BlockHash &block_hash,
      ApprovalDistribution::ImportedBlockInfo &&imported_block) {
    auto const block_tick =
        slot_number_to_tick(config_.slot_duration_millis, imported_block.slot);

    auto const no_show_duration =
        slot_number_to_tick(config_.slot_duration_millis,
                            imported_block.session_info.no_show_slots);

    auto const needed_approvals = imported_block.session_info.needed_approvals;
    auto const num_candidates = imported_block.included_candidates.size();

    scale::BitVec approved_bitfield;
    size_t num_ones = 0ull;

    if (0 == needed_approvals) {
      SL_DEBUG(logger_, "Auto-approving all candidates: {}", block_hash);
      approved_bitfield.bits.insert(
          approved_bitfield.bits.begin(), num_candidates, true);
      num_ones = num_candidates;
    } else {
      approved_bitfield.bits.insert(
          approved_bitfield.bits.begin(), num_candidates, false);
      for (size_t ix = 0; ix < imported_block.included_candidates.size();
           ++ix) {
        auto const &[_0, _1, _2, backing_group] =
            imported_block.included_candidates[ix];
        auto const backing_group_size =
            imported_block.session_info.validator_groups[backing_group].size();
        if (math::sat_sub_unsigned(imported_block.n_validators,
                                   backing_group_size)
            < needed_approvals) {
          num_ones += 1ull;
          approved_bitfield.bits[ix] = true;
        }
      }
    }

    if (num_ones == approved_bitfield.bits.size()) {
      ///TODO(iceseer): send Approved(block_hash)
    }

    /// TODO(iceseer): handle force_approved and maybe store in

    return BlockImportedCandidates{
        .block_hash = block_hash,
        .block_number = block_number,
        .block_tick = block_tick,
        .no_show_duration = no_show_duration,
        .imported_candidates = add_block_entry(
            block_number, block_hash, std::move(imported_block))};
  }

  template <typename Func>
  void ApprovalDistribution::handle_new_head(const primitives::BlockHash &head,
                                             primitives::BlockNumber finalized,
                                             Func &&func) {
    sequenceIgnore(
        this_context_->template wrap(
            asAsync([block_tree{block_tree_}, head]() mutable {
              return block_tree->getBlockHeader(head);
            })),
        int_pool_->io_context()->template wrap(asAsync(
            [head, wself{weak_from_this()}, func(std::forward<Func>(func))](
                auto &&header) mutable
            -> outcome::result<void> {
              auto self = wself.lock();
              if (!self) {
                return Error::NO_INSTANCE;
              }

              auto const block_number = header.number;
              self->imported_block_info(
                  head,
                  std::move(header),
                  self->this_context_,
                  [wself,
                   block_hash{head},
                   block_number,
                   func(std::forward<Func>(func))](
                      outcome::result<ImportedBlockInfo> &&block_info) mutable {
                    auto self = wself.lock();
                    if (!self) {
                      return;
                    }

                    if (block_info.has_error()) {
                      SL_WARN(self->logger_,
                              "ImportedBlockInfo request failed: {}",
                              block_info.error().message());
                      return;
                    }

                    std::forward<Func>(func)(self->processImportedBlock(
                        block_number,
                        block_hash,
                        std::move(block_info.value())));
                  });
              return outcome::success();
            })));
  }

  void ApprovalDistribution::on_active_leaves_update(
      primitives::BlockNumber finalized,
      std::vector<primitives::BlockHash> const &heads) {
    for (auto const &head : heads) {
      handle_new_head(
          head,
          finalized,
          [wself{weak_from_this()}, head](auto &&candidate) {
            if (auto self = wself.lock())
              self->scheduleTranche(head, std::move(candidate));
          });
    }
  }

  void ApprovalDistribution::scheduleTranche(
      primitives::BlockHash const &head, BlockImportedCandidates &&candidate) {
    SL_DEBUG(logger_,
             "Imported new block {}:{} with candidates count {}",
             candidate.block_number,
             candidate.block_hash,
             candidate.imported_candidates.size());

    for (auto const &[c_hash, c_entry] : candidate.imported_candidates) {
      auto const &block_assignments = c_entry.block_assignments.at(head);
      if (block_assignments.our_assignment) {
        auto const our_tranche = block_assignments.our_assignment->tranche;
        auto const tick = our_tranche + candidate.block_tick;
        SL_DEBUG(logger_,
                 "Scheduling first wakeup for block {}, tranche {} "
                 "after {}.",
                 candidate.block_hash,
                 our_tranche,
                 tick);

        // Our first wakeup will just be the tranche of our
        // assignment, if any. This will likely be superseded by
        // incoming assignments and approvals which trigger
        // rescheduling.
        // self->timer_->expiresAfter(std::chrono::milliseconds(tick));
        timer_->asyncWait([wself{weak_from_this()},
                           block_hash{candidate.block_hash},
                           block_number{candidate.block_number},
                           candidate_hash{c_hash}](auto &&ec) {
          if (auto self = wself.lock()) {
            if (ec) {
              SL_ERROR(self->logger_,
                       "error happened while waiting on tranche the "
                       "timer: {}",
                       ec.message());
              return;
            }

            self->handleTranche(block_hash, block_number, candidate_hash);
          }
        });
      }
    }
  }

  void ApprovalDistribution::handleTranche(
      const primitives::BlockHash &block_hash,
      primitives::BlockNumber block_number,
      const CandidateHash &candidate_hash) {
    /// By rescheduling we should accumulate approving data and send it.
    /// TODO(iceseer): not implemented
  }

}  // namespace kagome::parachain
