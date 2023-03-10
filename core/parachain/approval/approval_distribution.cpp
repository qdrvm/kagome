/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "clock/impl/basic_waitable_timer.hpp"
#include "common/visitor.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "crypto/crypto_store.hpp"
#include "crypto/hasher.hpp"
#include "crypto/sr25519_provider.hpp"
#include "network/peer_manager.hpp"
#include "network/router.hpp"
#include "parachain/approval/approval.hpp"
#include "parachain/approval/approval_distribution.hpp"
#include "parachain/approval/state.hpp"
#include "primitives/authority.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"
#include "utils/async_sequence.hpp"

#define _STRINGIZE(s) #s

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
    case E::ENTRY_IS_NOT_FOUND:
      return "Entry is not found";
    case E::ALREADY_APPROVING:
      return "Block in progress";
    case E::VALIDATOR_INDEX_OUT_OF_BOUNDS:
      return "Validator index out of bounds";
    case E::CORE_INDEX_OUT_OF_BOUNDS:
      return "Core index out of bounds";
    case E::IS_IN_BACKING_GROUP:
      return "Is in backing group";
    case E::SAMPLE_OUT_OF_BOUNDS:
      return "Sample is out of bounds";
    case E::VRF_DELAY_CORE_INDEX_MISMATCH:
      return "VRF delay core index mismatch";
    case E::VRF_VERIFY_AND_GET_TRANCHE:
      return "VRF verify and get tranche failed";
  }
  return "Unknown approval-distribution error";
}

static constexpr uint64_t kTickDurationMs = 500;
static constexpr kagome::network::Tick kApprovalDelay = 2;
static constexpr kagome::network::Tick kTickTooFarInFuture = 20;  // 10 seconds.

namespace {

  /// assumes `slot_duration_millis` evenly divided by tick duration.
  kagome::network::Tick slotNumberToTick(
      uint64_t slot_duration_millis,
      kagome::consensus::babe::BabeSlotNumber slot) {
    auto ticks_per_slot = slot_duration_millis / kTickDurationMs;
    return slot * ticks_per_slot;
  }

  kagome::network::Tick tickNow() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
               .count()
         / kTickDurationMs;
  }

  kagome::parachain::approval::DelayTranche trancheNow(
      uint64_t slot_duration_millis,
      kagome::consensus::babe::BabeSlotNumber base_slot) {
    return static_cast<kagome::parachain::approval::DelayTranche>(
        kagome::math::sat_sub_unsigned(
            tickNow(), slotNumberToTick(slot_duration_millis, base_slot)));
  }

  bool isInBackingGroup(
      std::vector<std::vector<kagome::parachain::ValidatorIndex>> const
          &validator_groups,
      kagome::parachain::ValidatorIndex validator,
      kagome::parachain::GroupIndex group) {
    if (group < validator_groups.size()) {
      for (auto const &i : validator_groups[group]) {
        if (i == validator) {
          return true;
        }
      }
    }
    return false;
  }

  void computeVrfModuloAssignments(
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

  void computeVrfDelayAssignments(
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

  /// Determine the amount of tranches of assignments needed to determine
  /// approval of a candidate.
  kagome::parachain::approval::RequiredTranches tranchesToApprove(
      kagome::parachain::ApprovalDistribution::ApprovalEntry const
          &approval_entry,
      scale::BitVec const &approvals,
      kagome::network::DelayTranche tranche_now,
      kagome::parachain::Tick block_tick,
      kagome::parachain::Tick no_show_duration,
      size_t needed_approvals) {
    auto const tick_now = tranche_now + block_tick;
    auto const n_validators = approval_entry.n_validators();

    std::optional<kagome::parachain::approval::State> state(needed_approvals);
    auto const &tranches = approval_entry.tranches;

    auto it = [&](uint32_t tranche)
        -> std::optional<kagome::parachain::approval::RequiredTranches> {
      auto s = *state;
      auto const clock_drift = s.depth * no_show_duration;
      auto const drifted_tick_now =
          kagome::math::sat_sub_unsigned(tick_now, clock_drift);
      auto const drifted_tranche_now =
          kagome::math::sat_sub_unsigned(drifted_tick_now, block_tick);

      if (tranche > drifted_tranche_now) {
        return std::nullopt;
      }

      size_t n_assignments = 0ull;
      std::optional<kagome::parachain::Tick> last_assignment_tick{};
      size_t no_shows = 0ull;
      std::optional<uint64_t> next_no_show{};

      if (auto i = std::lower_bound(
              tranches.begin(),
              tranches.end(),
              tranche,
              [](auto const &te, auto const t) { return te.tranche < t; });
          i != tranches.end() && i->tranche == tranche) {
        for (auto const &[v_index, t] : i->assignments) {
          if (v_index < n_validators) {
            ++n_assignments;
          }
          last_assignment_tick =
              std::max(t,
                       last_assignment_tick ? *last_assignment_tick
                                            : kagome::parachain::Tick{0ull});
          auto const no_show_at = kagome::math::sat_sub_unsigned(
                                      std::max(t, block_tick), clock_drift)
                                + no_show_duration;
          if (v_index < approvals.bits.size()) {
            auto const has_approved = approvals.bits.at(v_index);
            auto const is_no_show =
                !has_approved && no_show_at <= drifted_tick_now;
            if (!is_no_show && !has_approved) {
              next_no_show =
                  std::min(no_show_at + clock_drift,
                           next_no_show ? *next_no_show : uint64_t{0ull});
            }
            if (is_no_show) {
              ++no_shows;
            }
          }
        }
      }

      s = s.advance(
          n_assignments, no_shows, next_no_show, last_assignment_tick);
      auto const output =
          s.output(tranche, needed_approvals, n_validators, no_show_duration);

      state = kagome::visit_in_place(
          output,
          [&](const kagome::parachain::approval::PendingRequiredTranche &)
              -> std::optional<kagome::parachain::approval::State> {
            return s;
          },
          [](const auto &)
              -> std::optional<kagome::parachain::approval::State> {
            return std::nullopt;
          });

      return output;
    };

    uint32_t tranche = 0ul;
    kagome::parachain::approval::RequiredTranches required_tranches;
    while (auto req_trn = it(tranche++)) {
      required_tranches = *req_trn;
    }

    return required_tranches;
  }

  scale::BitVec &filter(scale::BitVec &lh, scale::BitVec const &rh) {
    BOOST_ASSERT(lh.bits.size() == rh.bits.size());
    for (size_t ix = 0; ix < lh.bits.size(); ++ix) {
      lh.bits[ix] = (lh.bits[ix] && rh.bits[ix]);
    }
    return lh;
  }

  kagome::parachain::approval::Check checkApproval(
      kagome::parachain::ApprovalDistribution::CandidateEntry const &candidate,
      kagome::parachain::ApprovalDistribution::ApprovalEntry const &approval,
      kagome::parachain::approval::RequiredTranches const &required) {
    auto const &approvals = candidate.approvals;
    if (3 * kagome::parachain::approval::count_ones(approvals)
        > approvals.bits.size()) {
      return kagome::parachain::approval::ApprovedOneThird{};
    }

    if (auto pending{
            boost::get<kagome::parachain::approval::PendingRequiredTranche>(
                &required)}) {
      return kagome::parachain::approval::Unapproved{};
    }
    if (auto all{boost::get<kagome::parachain::approval::AllRequiredTranche>(
            &required)}) {
      return kagome::parachain::approval::Unapproved{};
    }
    if (auto exact{
            boost::get<kagome::parachain::approval::ExactRequiredTranche>(
                &required)}) {
      auto assigned_mask = approval.assignments_up_to(exact->needed);
      auto const &approvals = candidate.approvals;
      auto const n_assigned =
          kagome::parachain::approval::count_ones(assigned_mask);
      filter(assigned_mask, approvals);
      auto const n_approved =
          kagome::parachain::approval::count_ones(assigned_mask);
      if (n_approved + exact->tolerated_missing >= n_assigned) {
        return std::make_pair(exact->tolerated_missing,
                              exact->last_assignment_tick);
      } else {
        return kagome::parachain::approval::Unapproved{};
      }
    }
    UNREACHABLE;
  }

  bool shouldTriggerAssignment(
      kagome::parachain::ApprovalDistribution::ApprovalEntry const
          &approval_entry,
      kagome::parachain::ApprovalDistribution::CandidateEntry const
          &candidate_entry,
      kagome::parachain::approval::RequiredTranches const &required_tranches,
      kagome::network::DelayTranche const tranche_now) {
    if (!approval_entry.our_assignment) {
      return false;
    }
    if (approval_entry.our_assignment->triggered) {
      return false;
    }
    if (approval_entry.our_assignment->tranche == 0) {
      return true;
    }
    if (auto all{boost::get<kagome::parachain::approval::AllRequiredTranche>(
            &required_tranches)}) {
      return !kagome::parachain::approval::is_approved(
          checkApproval(candidate_entry,
                        approval_entry,
                        kagome::parachain::approval::AllRequiredTranche{}),
          std::numeric_limits<kagome::network::Tick>::max());
    }
    if (auto pending{
            boost::get<kagome::parachain::approval::PendingRequiredTranche>(
                &required_tranches)}) {
    }
    if (auto exact{
            boost::get<kagome::parachain::approval::ExactRequiredTranche>(
                &required_tranches)}) {
      return false;
    }
    UNREACHABLE;
  }

  outcome::result<kagome::network::DelayTranche> checkAssignmentCert(
      kagome::network::CoreIndex claimed_core_index,
      kagome::network::ValidatorIndex validator_index,
      const kagome::runtime::SessionInfo &config,
      RelayVRFStory const &relay_vrf_story,
      kagome::parachain::approval::AssignmentCert const &assignment,
      kagome::network::GroupIndex backing_group) {
    using namespace kagome;
    using AD = parachain::ApprovalDistribution;

    if (validator_index >= config.assignment_keys.size()) {
      return AD::Error::VALIDATOR_INDEX_OUT_OF_BOUNDS;
    }

    auto const &validator_public = config.assignment_keys[validator_index];
    //    OUTCOME_TRY(pk, network::ValidatorId::fromSpan(validator_public));

    if (claimed_core_index >= config.n_cores) {
      return AD::Error::CORE_INDEX_OUT_OF_BOUNDS;
    }

    auto const is_in_backing = isInBackingGroup(
        config.validator_groups, validator_index, backing_group);
    if (is_in_backing) {
      return AD::Error::IS_IN_BACKING_GROUP;
    }

    auto const &vrf_output = assignment.vrf.output;
    auto const &vrf_proof = assignment.vrf.proof;

    return visit_in_place(
        assignment.kind,
        [&](parachain::approval::RelayVRFModulo const &obj)
            -> outcome::result<kagome::network::DelayTranche> {
          auto const sample = obj.sample;
          if (sample >= config.relay_vrf_modulo_samples) {
            return AD::Error::SAMPLE_OUT_OF_BOUNDS;
          }
          /// TODO(iceseer): vrf_verify_extra check
          return network::DelayTranche(0ull);
        },
        [&](parachain::approval::RelayVRFDelay const &obj)
            -> outcome::result<kagome::network::DelayTranche> {
          auto const core_index = obj.core_index;
          if (core_index != claimed_core_index) {
            return AD::Error::VRF_DELAY_CORE_INDEX_MISMATCH;
          }

          network::DelayTranche tranche;
          if (SR25519_SIGNATURE_RESULT_OK
              != sr25519_vrf_verify_and_get_tranche(
                  validator_public.data(),
                  vrf_output.data(),
                  vrf_proof.data(),
                  config.n_delay_tranches,
                  config.zeroth_delay_tranche_width,
                  &relay_vrf_story,
                  core_index,
                  &tranche)) {
            return AD::Error::VRF_VERIFY_AND_GET_TRANCHE;
          }
          return tranche;
        });
  }

}  // namespace

namespace kagome::parachain {

  ApprovalDistribution::ApprovalDistribution(
      std::shared_ptr<runtime::BabeApi> babe_api,
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<ThreadPool> thread_pool,
      std::shared_ptr<runtime::ParachainHost> parachain_host,
      std::shared_ptr<consensus::babe::BabeUtil> babe_util,
      std::shared_ptr<crypto::CryptoStore> keystore,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<boost::asio::io_context> this_context,
      std::shared_ptr<network::PeerView> peer_view,
      std::shared_ptr<ParachainProcessorImpl> parachain_processor,
      std::shared_ptr<crypto::Sr25519Provider> crypto_provider,
      std::shared_ptr<network::PeerManager> pm,
      std::shared_ptr<network::Router> router,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<parachain::Pvf> pvf,
      std::shared_ptr<parachain::Recovery> recovery)
      : thread_pool_{std::move(thread_pool)},
        parachain_host_(std::move(parachain_host)),
        babe_util_(std::move(babe_util)),
        keystore_(std::move(keystore)),
        hasher_(std::move(hasher)),
        this_context_(std::move(this_context)),
        config_(ApprovalVotingSubsystem{.slot_duration_millis = 6'000}),
        peer_view_(std::move(peer_view)),
        parachain_processor_(std::move(parachain_processor)),
        crypto_provider_(std::move(crypto_provider)),
        pm_(std::move(pm)),
        router_(std::move(router)),
        babe_api_(std::move(babe_api)),
        block_tree_(std::move(block_tree)),
        pvf_(std::move(pvf)),
        recovery_(std::move(recovery)) {
    BOOST_ASSERT(thread_pool_);
    BOOST_ASSERT(parachain_host_);
    BOOST_ASSERT(babe_util_);
    BOOST_ASSERT(keystore_);
    BOOST_ASSERT(this_context_);
    BOOST_ASSERT(peer_view_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(parachain_processor_);
    BOOST_ASSERT(crypto_provider_);
    BOOST_ASSERT(pm_);
    BOOST_ASSERT(router_);
    BOOST_ASSERT(babe_api_);
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(pvf_);
    BOOST_ASSERT(recovery_);
    app_state_manager->takeControl(*this);
  }

  ApprovalDistribution::~ApprovalDistribution() {
    for (auto &it : active_tranches_) {
      BOOST_ASSERT(it.second);
    }
    active_tranches_.clear();
    BOOST_ASSERT(nullptr == this_context_);
  }

  bool ApprovalDistribution::prepare() {
    my_view_sub_ = std::make_shared<network::PeerView::MyViewSubscriber>(
        peer_view_->getMyViewObservable(), false);
    my_view_sub_->subscribe(my_view_sub_->generateSubscriptionSetId(),
                            network::PeerView::EventType::kViewUpdated);
    my_view_sub_->setCallback(
        [wptr{weak_from_this()}](auto /*set_id*/,
                                 auto && /*internal_obj*/,
                                 auto /*event_type*/,
                                 const network::ExView &event) {
          if (auto self = wptr.lock()) {
            self->on_active_leaves_update(event);
          }
        });

    return true;
  }

  bool ApprovalDistribution::start() {
    return true;
  }

  void ApprovalDistribution::stop() {}

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
      SL_TRACE(logger_,
               "Not producing assignments because config is degenerate "
               "(n_cores:{}, has_assignment_keys:{}, has_validator_groups:{})",
               config.n_cores,
               config.assignment_keys.size(),
               config.validator_groups.size());
      return {};
    }

    SL_INFO(logger_,
            "Compute assignments."
            "(n_cores:{}, has_assignment_keys:{}, has_validator_groups:{})",
            config.n_cores,
            config.assignment_keys.size(),
            config.validator_groups.size());

    std::optional<std::pair<ValidatorIndex, crypto::Sr25519Keypair>>
        founded_key = findAssignmentKey(keystore, config);
    if (!founded_key) {
      return {};
    }

    auto const &[validator_ix, assignments_key] = *founded_key;
    std::vector<CoreIndex> lc;
    for (auto const &[candidate_hash, _, core_ix, group_ix] : leaving_cores) {
      if (isInBackingGroup(config.validator_groups, validator_ix, group_ix)) {
        continue;
      }
      lc.push_back(core_ix);
    }

    common::Buffer keypair_buf{};
    keypair_buf.put(assignments_key.secret_key).put(assignments_key.public_key);

    std::unordered_map<CoreIndex, ApprovalDistribution::OurAssignment>
        assignments;
    computeVrfModuloAssignments(
        keypair_buf, config, relay_vrf_story, lc, validator_ix, assignments);
    computeVrfDelayAssignments(
        keypair_buf, config, relay_vrf_story, lc, validator_ix, assignments);

    return assignments;
  }

  void ApprovalDistribution::imported_block_info(
      const primitives::BlockHash &block_hash,
      const primitives::BlockHeader &block_header,
      const std::shared_ptr<boost::asio::io_context> &callback_exec_context) {
    BOOST_ASSERT(this_context_->get_executor().running_in_this_thread());
    request_included_candidates(block_hash);
    request_session_index_and_info(block_hash, block_header.parent_hash);
    request_babe_epoch_and_block_header(
        callback_exec_context, block_header, block_hash);
  }

  template <typename Func>
  void ApprovalDistribution::for_ACU(const primitives::BlockHash &block_hash,
                                     Func &&func) {
    BOOST_ASSERT(this_context_->get_executor().running_in_this_thread());
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

    ::RelayVRFStory relay_vrf;
    if (auto res = unsafe_vrf.compute_randomness(
            relay_vrf, *ac.authorities, *ac.randomness, *ac.babe_epoch);
        res.has_error()) {
      logger_->warn("Relay VRF return error.(error={})", res.error().message());
      return;
    }
    auto assignments = compute_assignments(
        keystore_, *ac.session_info, relay_vrf, *ac.included_candidates);

    /// TODO(iceseer): force approve impl

    ac.complete_callback(ImportedBlockInfo{
        .included_candidates = std::move(*ac.included_candidates),
        .session_index = *ac.session_index,
        .assignments = std::move(assignments),
        .n_validators = ac.session_info->validators.size(),
        .relay_vrf_story = relay_vrf,
        .slot = unsafe_vrf.slot,
        .session_info = std::move(*ac.session_info),
        .force_approve = std::nullopt});
  }

  std::optional<
      std::pair<std::reference_wrapper<
                    kagome::parachain::ApprovalDistribution::ApprovalEntry>,
                kagome::parachain::approval::ApprovalStatus>>
  ApprovalDistribution::approval_status(BlockEntry const &block_entry,
                                        CandidateEntry &candidate_entry) {
    auto &session_info = block_entry.session_info;
    auto const block_hash = block_entry.block_hash;

    auto const tranche_now =
        ::trancheNow(config_.slot_duration_millis, block_entry.slot);
    auto const block_tick =
        ::slotNumberToTick(config_.slot_duration_millis, block_entry.slot);
    auto const no_show_duration = ::slotNumberToTick(
        config_.slot_duration_millis, session_info.no_show_slots);

    if (auto approval_entry = candidate_entry.approval_entry(block_hash)) {
      auto required_tranches = tranchesToApprove(approval_entry->get(),
                                                 candidate_entry.approvals,
                                                 tranche_now,
                                                 block_tick,
                                                 no_show_duration,
                                                 session_info.needed_approvals);
      return std::make_pair(
          *approval_entry,
          approval::ApprovalStatus{
              .required_tranches = std::move(required_tranches),
              .tranche_now = tranche_now,
              .block_tick = block_tick,
          });
    }
    return std::nullopt;
  }

  void ApprovalDistribution::request_session_index_and_info(
      const primitives::BlockHash &block_hash,
      const primitives::BlockHash &parent_hash) {
    auto call = [&]() -> outcome::result<void> {
      OUTCOME_TRY(session_index,
                  parachain_host_->session_index_for_child(parent_hash));
      OUTCOME_TRY(session_info,
                  parachain_host_->session_info(block_hash, session_index));
      if (!session_info) {
        SL_ERROR(logger_,
                 "No session info for [session_index: {}, block_hash: {}]",
                 session_index,
                 block_hash);
        return Error::NO_SESSION_INFO;
      } else {
        SL_INFO(logger_,
                "Found session info. (block hash={}, session index={}, "
                "validators count={}, assignment keys count={}, "
                "availability cores={}, delay tranches ={})",
                block_hash,
                session_index,
                session_info->validators.size(),
                session_info->assignment_keys.size(),
                session_info->n_cores,
                session_info->n_delay_tranches);
      }

      for_ACU(block_hash, [&](auto &acu) {
        acu.second.session_index = session_index;
        acu.second.session_info = std::move(*session_info);
        try_process_approving_context(acu);
      });

      return outcome::success();
    };
    if (auto res = call(); res.has_error()) {
      logger_->warn("Error while retrieve session index and info.(error={})",
                    res.error().message());
    }
  }

  void ApprovalDistribution::request_babe_epoch_and_block_header(
      const std::shared_ptr<boost::asio::io_context> &exec_context,
      const primitives::BlockHeader &block_header,
      const primitives::BlockHash &block_hash) {
    auto call = [&]() -> outcome::result<void> {
      OUTCOME_TRY(babe_digests, consensus::babe::getBabeDigests(block_header));
      OUTCOME_TRY(babe_config, babe_api_->configuration(block_hash));
      const consensus::babe::EpochNumber epoch_number =
          babe_util_->slotToEpoch(babe_digests.second.slot_number);

      for_ACU(block_hash, [&](auto &acu) {
        acu.second.babe_epoch = epoch_number;
        acu.second.babe_block_header = std::move(babe_digests.second);
        acu.second.authorities = std::move(babe_config.authorities);
        acu.second.randomness = std::move(babe_config.randomness);

        try_process_approving_context(acu);
      });

      return outcome::success();
    };
    if (auto res = call(); res.has_error()) {
      logger_->warn(
          "Error while retrieve babe epoch and block header.(error={})",
          res.error().message());
    }
  }

  void ApprovalDistribution::request_included_candidates(
      const primitives::BlockHash &block_hash) {
    auto call = [&]() -> outcome::result<void> {
      OUTCOME_TRY(candidates, parachain_host_->candidate_events(block_hash));
      ApprovalDistribution::CandidateIncludedList included;

      for (auto &candidate : candidates) {
        if (auto obj{boost::get<runtime::CandidateIncluded>(&candidate)}) {
          included.emplace_back(std::make_tuple(
              hasher_->blake2b_256(
                  scale::encode(obj->candidate_receipt).value()),
              std::move(obj->candidate_receipt),
              obj->core_index,
              obj->group_index));
        }
      }

      for_ACU(block_hash, [&](auto &acu) {
        store_included_candidates(acu, std::move(included));
        try_process_approving_context(acu);
      });

      return outcome::success();
    };
    if (auto res = call(); res.has_error()) {
      logger_->warn("Error while retrieve included candidates.(error={})",
                    res.error().message());
    }
  }

  outcome::result<std::vector<
      std::pair<CandidateHash, ApprovalDistribution::CandidateEntry>>>
  ApprovalDistribution::add_block_entry(
      primitives::BlockNumber block_number,
      const primitives::BlockHash &block_hash,
      const primitives::BlockHash &parent_hash,
      scale::BitVec &&approved_bitfield,
      ImportedBlockInfo &&block_info) {
    std::vector<std::pair<CandidateHash, CandidateEntry>> entries;
    std::vector<std::pair<CoreIndex, CandidateHash>> candidates;
    if (auto blocks = storedBlocks().get_or_create(block_number);
        blocks.get().find(block_hash) != blocks.get().end()) {
      return entries;
    } else {
      blocks.get().insert(block_hash);
    }

    entries.reserve(block_info.included_candidates.size());
    candidates.reserve(block_info.included_candidates.size());
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
      candidates.emplace_back(coreIndex, candidateHash);
    }

    // Update the child index for the parent.
    if (auto parent = storedBlockEntries().get(parent_hash)) {
      parent->get().children.push_back(block_hash);
    }

    // Put the new block entry in.
    storedBlockEntries().set(
        block_hash,
        BlockEntry{.block_hash = block_hash,
                   .parent_hash = parent_hash,
                   .block_number = block_number,
                   .session = block_info.session_index,
                   .session_info = std::move(block_info.session_info),
                   .slot = block_info.slot,
                   .relay_vrf_story = std::move(block_info.relay_vrf_story),
                   .candidates = std::move(candidates),
                   .approved_bitfield = std::move(approved_bitfield),
                   .children = {}});

    return entries;
  }

  outcome::result<ApprovalDistribution::BlockImportedCandidates>
  ApprovalDistribution::processImportedBlock(
      primitives::BlockNumber block_number,
      const primitives::BlockHash &block_hash,
      const primitives::BlockHash &parent_hash,
      ApprovalDistribution::ImportedBlockInfo &&imported_block) {
    SL_TRACE(logger_,
             "Star imported block processing. (block number={}, block hash={}, "
             "parent hash={})",
             block_number,
             block_hash,
             parent_hash);

    auto const block_tick =
        slotNumberToTick(config_.slot_duration_millis, imported_block.slot);

    auto const no_show_duration =
        slotNumberToTick(config_.slot_duration_millis,
                         imported_block.session_info.no_show_slots);

    auto const needed_approvals = imported_block.session_info.needed_approvals;
    auto const num_candidates = imported_block.included_candidates.size();

    scale::BitVec approved_bitfield;
    size_t num_ones = 0ull;

    if (0 == needed_approvals) {
      SL_TRACE(logger_, "Auto-approving all candidates: {}", block_hash);
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
      notifyApproved(block_hash);
    }

    /// TODO(iceseer): handle force_approved and maybe store in
    SL_TRACE(logger_,
             "Add block entry. (block number={}, block hash={}, parent "
             "hash={}, num candidates={})",
             block_number,
             block_hash,
             parent_hash,
             num_candidates);
    OUTCOME_TRY(entries,
                add_block_entry(block_number,
                                block_hash,
                                parent_hash,
                                std::move(approved_bitfield),
                                std::move(imported_block)));

    std::vector<CandidateHash> candidates;
    for (auto const &[hash, _0, _1, _2] : imported_block.included_candidates) {
      candidates.emplace_back(hash);
    }

    runNewBlocks(approval::BlockApprovalMeta{
        .hash = block_hash,
        .number = block_number,
        .parent_hash = parent_hash,
        .candidates = std::move(candidates),
        .slot = imported_block.slot,
        .session = imported_block.session_index,
    });

    return BlockImportedCandidates{.block_hash = block_hash,
                                   .block_number = block_number,
                                   .block_tick = block_tick,
                                   .no_show_duration = no_show_duration,
                                   .imported_candidates = std::move(entries)};
  }

  void ApprovalDistribution::runNewBlocks(approval::BlockApprovalMeta &&meta) {
    std::optional<primitives::BlockHash> new_hash;
    if (!storedDistribBlockEntries().get(meta.hash)) {
      auto const candidates_count = meta.candidates.size();
      std::vector<DistribCandidateEntry> candidates;
      candidates.resize(candidates_count);

      new_hash = meta.hash;
      storedDistribBlockEntries().set(meta.hash,
                                      DistribBlockEntry{
                                          .candidates = std::move(candidates),
                                      });
    }

    logger_->trace("Got new block.(hash={})", new_hash);
    /// TODO(iceseer): intersection in views

    for (auto it = pending_known_.begin(); it != pending_known_.end();) {
      if (!storedDistribBlockEntries().get(it->first)) {
        ++it;
      } else {
        logger_->trace("Processing pending assignment/approvals.(count={})",
                       it->second.size());
        for (auto i = it->second.begin(); i != it->second.end(); ++i) {
          visit_in_place(
              i->second,
              [&](network::Assignment const &assignment) {
                import_and_circulate_assignment(
                    i->first,
                    assignment.indirect_assignment_cert,
                    assignment.candidate_ix);
              },
              [&](network::IndirectSignedApprovalVote const &approval) {
                import_and_circulate_approval(i->first, approval);
              });
        }
        it = pending_known_.erase(it);
      }
    }
  }

  template <typename Func>
  void ApprovalDistribution::handle_new_head(const primitives::BlockHash &head,
                                             const network::ExView &updated,
                                             Func &&func) {
    /*sequenceIgnore(this_context_->template wrap(asAsync(
        [head,
         updated,
         wself{weak_from_this()},
         func(std::forward<Func>(func))]() mutable -> outcome::result<void> {
          auto self = wself.lock();
          if (!self) {
            return Error::NO_INSTANCE;
          }*/
    BOOST_ASSERT(this_context_->get_executor().running_in_this_thread());

    /// clear unuseful heads
    for (auto const &l_head : updated.lost) {
      approving_context_map_.erase(l_head);
    }

    auto const block_number = updated.new_head.number;
    auto parent_hash{updated.new_head.parent_hash};
    if (approving_context_map_.count(head) != 0ull) {
      logger_->warn("Approving {} already in progress.", head);
      return;  // Error::ALREADY_APPROVING;
    }

    approving_context_map_.emplace(
        head,
        ApprovingContext{
            .block_header = updated.new_head,
            .included_candidates = std::nullopt,
            .session_index = std::nullopt,
            .babe_block_header = std::nullopt,
            .babe_epoch = std::nullopt,
            .session_info = std::nullopt,
            .complete_callback_context = this_context_,
            .complete_callback =
                [wself{weak_from_this()},
                 block_hash{head},
                 block_number,
                 parent_hash{std::move(parent_hash)},
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
                      parent_hash,
                      std::move(block_info.value())));
                }});

    imported_block_info(head, std::move(updated.new_head), this_context_);
    // return outcome::success();
    //})));
  }

  void ApprovalDistribution::on_active_leaves_update(
      const network::ExView &updated) {
    if (auto result =
            primitives::calculateBlockHash(updated.new_head, *hasher_)) {
      if (!storedDistribBlockEntries().get(result.value())) {
        [[maybe_unused]] auto &_ = pending_known_[result.value()];
      }

      for (auto it = pending_known_.begin(); it != pending_known_.end();) {
        if (updated.view.contains(it->first)) {
          ++it;
        } else {
          logger_->trace("Cleaning up stale pending messages.(block hash={})",
                         it->first);
          it = pending_known_.erase(it);
        }
      }

      handle_new_head(
          result.value(),
          updated,
          [wself{weak_from_this()},
           head{result.value()}](auto &&possible_candidate) {
            if (auto self = wself.lock()) {
              if (possible_candidate.has_error()) {
                SL_ERROR(self->logger_,
                         "Internal error while retrieve block imported "
                         "candidates: {}",
                         possible_candidate.error().message());
                return;
              }

              BOOST_ASSERT(
                  self->this_context_->get_executor().running_in_this_thread());
              self->scheduleTranche(head,
                                    std::move(possible_candidate.value()));
            }
          });
    } else {
      logger_->error("Block header hashing failed: {}",
                     result.error().message());
    }
  }

  void ApprovalDistribution::launch_approval(
      RelayHash const &relay_block_hash,
      CandidateHash const &candidate_hash,
      SessionIndex session_index,
      network::CandidateReceipt const &candidate,
      ValidatorIndex validator_index,
      Hash block_hash,
      GroupIndex backing_group) {
    auto on_recover_complete =
        [wself{weak_from_this()},
         candidate,
         block_hash,
         session_index,
         validator_index,
         candidate_hash,
         relay_block_hash](
            std::optional<outcome::result<runtime::AvailableData>>
                &&opt_result) mutable {
          auto self = wself.lock();
          if (!self) {
            return;
          }

          if (!opt_result) {
            self->logger_->warn(
                "No available parachain data.(session index={}, candidate "
                "hash={}, relay block hash={})",
                session_index,
                candidate_hash,
                relay_block_hash);
            return;
          }

          if (opt_result->has_error()) {
            self->logger_->warn(
                "Parachain data recovery failed.(error={}, session index={}, "
                "candidate hash={}, relay block hash={})",
                opt_result->error().message(),
                session_index,
                candidate_hash,
                relay_block_hash);
            return;
          }
          auto &available_data = opt_result->value();
          BOOST_ASSERT(
              self->this_context_->get_executor().running_in_this_thread());
          [[maybe_unused]] auto const para_id = candidate.descriptor.para_id;

          auto result = self->parachain_host_->validation_code_by_hash(
              block_hash, candidate.descriptor.validation_code_hash);
          if (result.has_error() || !result.value()) {
            self->logger_->warn(
                "Approval state is failed. Block hash {}, session index {}, "
                "validator index {}, relay parent {}",
                block_hash,
                session_index,
                validator_index,
                candidate.descriptor.relay_parent);
            return;  /// ApprovalState::failed
          }

          self->logger_->info(
              "Make exhaustive validation. Candidate hash {}, validator index "
              "{}, block hash {}",
              candidate_hash,
              block_hash);

          runtime::ValidationCode &validation_code = *result.value();
          if (ApprovalOutcome::Approved
              == self->validate_candidate_exhaustive(
                  available_data.validation_data,
                  available_data.pov,
                  candidate,
                  validation_code)) {
            self->this_context_->wrap(  /// IssueApproval
                [wself, candidate_hash, validator_index, relay_block_hash]() {
                  if (auto self = wself.lock()) {
                    self->issue_approval(
                        candidate_hash, validator_index, relay_block_hash);
                  }
                });
          }
        };

    recovery_->recover(candidate,
                       session_index,
                       backing_group,
                       std::move(on_recover_complete));
  }

  ApprovalDistribution::ApprovalOutcome
  ApprovalDistribution::validate_candidate_exhaustive(
      const runtime::PersistedValidationData &data,
      const network::ParachainBlock &pov,
      const network::CandidateReceipt &receipt,
      const ParachainRuntime &code) {
    if (auto result = pvf_->pvfValidate(data, pov, receipt, code);
        result.has_error()) {
      logger_->warn(
          "Approval validation failed.(parachain id={}, relay parent={})",
          receipt.descriptor.para_id,
          receipt.descriptor.relay_parent);
      return ApprovalOutcome::Failed;
    }
    return ApprovalOutcome::Approved;
  }

#define GET_OPT_VALUE_OR_EXIT(name, err, ...)       \
  auto __##name = (__VA_ARGS__);                    \
  if (!__##name) {                                  \
    logger_->warn("Initialize __" #name "failed."); \
    return (err);                                   \
  }                                                 \
  auto &name = __##name->get();

  ApprovalDistribution::AssignmentCheckResult
  ApprovalDistribution::check_and_import_assignment(
      approval::IndirectAssignmentCert const &assignment,
      CandidateIndex candidate_index) {
    BOOST_ASSERT(this_context_->get_executor().running_in_this_thread());
    auto const tick_now = ::tickNow();

    GET_OPT_VALUE_OR_EXIT(block_entry,
                          AssignmentCheckResult::Bad,
                          storedBlockEntries().get(assignment.block_hash));
    auto &session_info = block_entry.session_info;
    if (candidate_index >= block_entry.candidates.size()) {
      logger_->warn(
          "Candidate index more than candidates array.(candidate index={})",
          candidate_index);
      return AssignmentCheckResult::Bad;
    }

    auto &[claimed_core_index, assigned_candidate_hash] =
        block_entry.candidates[candidate_index];

    GET_OPT_VALUE_OR_EXIT(
        candidate_entry,
        AssignmentCheckResult::Bad,
        storedCandidateEntries().get(assigned_candidate_hash));
    GET_OPT_VALUE_OR_EXIT(
        approval_entry,
        AssignmentCheckResult::Bad,
        candidate_entry.approval_entry(assignment.block_hash));

    DelayTranche tranche;
    if (auto res = checkAssignmentCert(claimed_core_index,
                                       assignment.validator,
                                       session_info,
                                       block_entry.relay_vrf_story,
                                       assignment.cert,
                                       approval_entry.backing_group);
        res.has_value()) {
      auto const current_tranche =
          ::trancheNow(config_.slot_duration_millis, block_entry.slot);
      auto const too_far_in_future =
          current_tranche + DelayTranche(kTickTooFarInFuture);
      if (res.value() >= too_far_in_future) {
        return AssignmentCheckResult::TooFarInFuture;
      }
      tranche = res.value();
    } else {
      logger_->warn(
          "Check assignment certificate failed.(error={}, candidate index={})",
          res.error().message(),
          candidate_index);
      return AssignmentCheckResult::Bad;
    }

    auto const is_duplicate = approval_entry.is_assigned(assignment.validator);
    approval_entry.import_assignment(tranche, assignment.validator, tick_now);

    AssignmentCheckResult res;
    if (is_duplicate) {
      res = AssignmentCheckResult::AcceptedDuplicate;
    } else {
      logger_->trace(
          "Imported assignment. (validator={}, candidate hash={}, para id={})",
          assignment.validator,
          assigned_candidate_hash,
          candidate_entry.candidate.descriptor.para_id);
      res = AssignmentCheckResult::Accepted;
    }

    if (auto result = approval_status(block_entry, candidate_entry); result) {
      schedule_wakeup_action(result->first.get(),
                             block_entry.block_hash,
                             block_entry.block_number,
                             assigned_candidate_hash,
                             result->second.block_tick,
                             tick_now,
                             result->second.required_tranches);
    }

    /// TODO(iceseer): new candidate must be in db already
    return res;
  }

  ApprovalDistribution::ApprovalCheckResult
  ApprovalDistribution::check_and_import_approval(
      network::IndirectSignedApprovalVote const &approval) {
    GET_OPT_VALUE_OR_EXIT(
        block_entry,
        ApprovalCheckResult::Bad,
        storedBlockEntries().get(approval.payload.payload.block_hash));
    auto &session_info = block_entry.session_info;
    if (approval.payload.payload.candidate_index
        >= block_entry.candidates.size()) {
      logger_->warn(
          "Candidate index more than candidates array.(candidate index={})",
          approval.payload.payload.candidate_index);
      return ApprovalCheckResult::Bad;
    }

    auto const &approved_candidate_hash =
        block_entry.candidates[approval.payload.payload.candidate_index].second;
    if (approval.payload.ix >= session_info.validators.size()) {
      logger_->warn(
          "Validator index more than validators array.(validator index={})",
          approval.payload.ix);
      return ApprovalCheckResult::Bad;
    }

    auto const &pubkey = session_info.validators[approval.payload.ix];
    GET_OPT_VALUE_OR_EXIT(
        candidate_entry,
        ApprovalCheckResult::Bad,
        storedCandidateEntries().get(approved_candidate_hash));

    if (auto ae = candidate_entry.approval_entry(
            approval.payload.payload.block_hash)) {
      if (!ae->get().is_assigned(approval.payload.ix)) {
        logger_->warn(
            "No assignment from validator.(block hash={}, candidate hash={}, "
            "validator={})",
            approval.payload.payload.block_hash,
            approved_candidate_hash,
            approval.payload.ix);
        return ApprovalCheckResult::Bad;
      }
    } else {
      logger_->error("No approval entry.(block hash={}, candidate hash={})",
                     approval.payload.payload.block_hash,
                     approved_candidate_hash);
      return ApprovalCheckResult::Bad;
    }

    logger_->info(
        "Importing approval vote.(validator index={}, validator id={}, "
        "candidate hash={}, para id={})",
        approval.payload.ix,
        pubkey,
        approved_candidate_hash,
        candidate_entry.candidate.descriptor.para_id);
    advance_approval_state(block_entry,
                           approved_candidate_hash,
                           candidate_entry,
                           approval::RemoteApproval{
                               .validator_ix = approval.payload.ix,
                           });

    return ApprovalCheckResult::Accepted;
  }

#undef GET_OPT_VALUE_OR_EXIT

  void ApprovalDistribution::import_and_circulate_assignment(
      MessageSource const &source,
      approval::IndirectAssignmentCert const &assignment,
      CandidateIndex claimed_candidate_index) {
    BOOST_ASSERT(this_context_->get_executor().running_in_this_thread());
    auto const &block_hash = assignment.block_hash;
    auto const validator_index = assignment.validator;
    auto opt_entry = storedDistribBlockEntries().get(block_hash);
    if (!opt_entry) {
      logger_->warn(
          "Unexpected assignment. (peer id={}, block hash={}, validator "
          "index={})",
          source ? source->get().toBase58() : "our",
          block_hash,
          validator_index);
      return;
    }

    logger_->info(
        "Import assignment. (peer id={}, block hash={}, validator "
        "index={})",
        source ? source->get().toBase58() : "our",
        block_hash,
        validator_index);
    auto &entry = opt_entry->get();
    if (source) {
      /// TODO(iceseer): vector-clock for knowledge
      switch (
          check_and_import_assignment(assignment, claimed_candidate_index)) {
        case AssignmentCheckResult::Accepted:
          break;
        case AssignmentCheckResult::Bad:
          SL_WARN(logger_,
                  "Got bad assignment from peer. (peer id={}, block hash={})",
                  source->get(),
                  block_hash);
          return;
        case AssignmentCheckResult::TooFarInFuture:
          SL_TRACE(
              logger_,
              "Got an assignment too far in the future. (peer id={}, block "
              "hash={})",
              source->get(),
              block_hash);
          return;
        case AssignmentCheckResult::AcceptedDuplicate:
          SL_TRACE(logger_,
                   "Got duplicated assignment. (peer id={}, block hash={})",
                   source->get(),
                   block_hash);
          return;
      }
    } else {
      /// TODO(iceseer): vector-clock for knowledge
    }

    auto const local = !source;
    if (claimed_candidate_index >= entry.candidates.size()) {
      logger_->warn(
          "Unexpected candidate entry. (candidate index={}, block hash={})",
          claimed_candidate_index,
          block_hash);
      return;
    }

    auto &candidate_entry = entry.candidates[claimed_candidate_index];
    [[maybe_unused]] auto &message_state =
        candidate_entry.messages
            .emplace(validator_index,
                     MessageState{
                         .approval_state = assignment.cert,
                         .local = local,
                     })
            .first->second;

    runDistributeAssignment(assignment, claimed_candidate_index);
  }

  void ApprovalDistribution::import_and_circulate_approval(
      MessageSource const &source,
      network::IndirectSignedApprovalVote const &vote) {
    BOOST_ASSERT(this_context_->get_executor().running_in_this_thread());
    auto const &block_hash = vote.payload.payload.block_hash;
    auto const validator_index = vote.payload.ix;
    auto const candidate_index = vote.payload.payload.candidate_index;
    auto opt_entry = storedDistribBlockEntries().get(block_hash);
    if (!opt_entry) {
      logger_->info(
          "Unexpected approval. (peer id={}, block hash={}, validator "
          "index={})",
          source ? source->get().toBase58() : "our",
          block_hash,
          validator_index);
      return;
    }

    logger_->info(
        "Import approval. (peer id={}, block hash={}, validator "
        "index={})",
        source ? source->get().toBase58() : "our",
        block_hash,
        validator_index);

    auto &entry = opt_entry->get();
    if (source) {
      /// TODO(iceseer): vector-clock for knowledge
      switch (check_and_import_approval(vote)) {
        case ApprovalCheckResult::Accepted:
          break;
        case ApprovalCheckResult::Bad:
          logger_->warn(
              "Got bad approval from peer. (peer id={}, block hash={})",
              source->get(),
              block_hash);
          return;
      }
    } else {
      /// TODO(iceseer): vector-clock for knowledge
    }

    if (candidate_index >= entry.candidates.size()) {
      logger_->warn(
          "Unexpected candidate entry in import approval. (candidate index={}, "
          "block hash={}, validator index={})",
          candidate_index,
          block_hash,
          validator_index);
      return;
    }

    auto &candidate_entry = entry.candidates[candidate_index];
    if (auto it = candidate_entry.messages.find(validator_index);
        it != candidate_entry.messages.end()) {
      auto cert{
          boost::get<DistribApprovalStateAssigned>(&it->second.approval_state)};
      BOOST_ASSERT(cert);
      it->second.approval_state =
          DistribApprovalStateApproved{*cert, vote.signature};
    } else {
      logger_->warn(
          "Importing an approval we don't have an assignment for. (candidate "
          "index={}, block hash={}, validator index={})",
          candidate_index,
          block_hash,
          validator_index);
      return;
    }
    runDistributeApproval(vote);
  }

  void ApprovalDistribution::onValidationProtocolMsg(
      libp2p::peer::PeerId const &peer_id,
      network::ValidatorProtocolMessage const &message) {
    if (auto m{boost::get<network::ApprovalDistributionMessage>(&message)}) {
      visit_in_place(
          *m,
          [&](network::Assignments const &assignments) {
            logger_->info("Received assignments.(peer_id={}, count={})",
                          peer_id,
                          assignments.assignments.size());
            for (auto const &assignment : assignments.assignments) {
              if (auto it = pending_known_.find(
                      assignment.indirect_assignment_cert.block_hash);
                  it != pending_known_.end()) {
                logger_->trace(
                    "Pending assignment.(block hash={}, claimed index={}, "
                    "validator={}, peer={})",
                    assignment.indirect_assignment_cert.block_hash,
                    assignment.candidate_ix,
                    assignment.indirect_assignment_cert.validator,
                    peer_id);
                it->second.emplace_back(
                    std::make_pair(peer_id, PendingMessage{assignment}));
                continue;
              }

              import_and_circulate_assignment(
                  peer_id,
                  assignment.indirect_assignment_cert,
                  assignment.candidate_ix);
            }
          },
          [&](network::Approvals const &approvals) {
            logger_->info("Received approvals.(peer_id={}, count={})",
                          peer_id,
                          approvals.approvals.size());
            for (auto const &approval_vote : approvals.approvals) {
              if (auto it = pending_known_.find(
                      approval_vote.payload.payload.block_hash);
                  it != pending_known_.end()) {
                logger_->trace(
                    "Pending approval.(block hash={}, candidate index={}, "
                    "validator={}, peer={})",
                    approval_vote.payload.payload.block_hash,
                    approval_vote.payload.payload.candidate_index,
                    approval_vote.payload.ix,
                    peer_id);
                it->second.emplace_back(
                    std::make_pair(peer_id, PendingMessage{approval_vote}));
                continue;
              }

              import_and_circulate_approval(peer_id, approval_vote);
            }
          },
          [&](auto const &) { UNREACHABLE; });
    }
  }

  void ApprovalDistribution::runDistributeAssignment(
      approval::IndirectAssignmentCert const &indirect_cert,
      CandidateIndex candidate_index) {
    logger_->info(
        "Distributing assignment on candidate (block hash={}, candidate "
        "index={})",
        indirect_cert.block_hash,
        candidate_index);

    auto se = pm_->getStreamEngine();
    BOOST_ASSERT(se);

    se->broadcast(router_->getValidationProtocol(),
                  std::make_shared<
                      network::WireMessage<network::ValidatorProtocolMessage>>(
                      network::ApprovalDistributionMessage{network::Assignments{
                          .assignments = {network::Assignment{
                              .indirect_assignment_cert = indirect_cert,
                              .candidate_ix = candidate_index,
                          }}}}));
  }

  void ApprovalDistribution::runDistributeApproval(
      network::IndirectSignedApprovalVote const &vote) {
    logger_->info(
        "Distributing our approval vote on candidate (block={}, index={})",
        vote.payload.payload.block_hash,
        vote.payload.payload.candidate_index);

    auto se = pm_->getStreamEngine();
    BOOST_ASSERT(se);

    se->broadcast(router_->getValidationProtocol(),
                  std::make_shared<
                      network::WireMessage<network::ValidatorProtocolMessage>>(
                      network::ApprovalDistributionMessage{network::Approvals{
                          .approvals = {vote},
                      }}));
  }

  void ApprovalDistribution::issue_approval(CandidateHash const &candidate_hash,
                                            ValidatorIndex validator_index,
                                            RelayHash const &block_hash) {
    auto be = storedBlockEntries().get(block_hash);
    if (!be) {
      logger_->info("No block entry for {}. Staled.", block_hash);
      return;
    }

    auto &block_entry = be->get();
    auto candidate_index = block_entry.candidateIxByHash(candidate_hash);
    if (!candidate_index) {
      logger_->warn(
          "Candidate hash {} is not present in the block entry's candidates "
          "for relay block {}",
          candidate_hash,
          block_entry.parent_hash);
      return;
    }

    auto &session_info = block_entry.session_info;
    if (*candidate_index >= block_entry.candidates.size()) {
      logger_->warn(
          "Received malformed request to approve out-of-bounds candidate index "
          "{} included at block {}",
          *candidate_index,
          block_hash);
      return;
    }

    auto const &c_hash = block_entry.candidates[*candidate_index].second;
    auto r = storedCandidateEntries().get(c_hash);
    if (!r) {
      logger_->warn("Missing entry for candidate index {} included at block {}",
                    *candidate_index,
                    block_hash);
      return;
    }

    auto &candidate_entry = *r;
    if (validator_index >= session_info.validators.size()) {
      logger_->warn("Validator index {} out of bounds in session {}",
                    validator_index,
                    block_entry.session);
      return;
    }

    auto &validator_pubkey = session_info.validators[validator_index];
    auto const session = block_entry.session;
    auto sig = sign_approval(validator_pubkey, session, c_hash);
    if (!sig) {
      logger_->warn("Could not issue approval signature for pubkey {}",
                    validator_pubkey);
      return;
    }

    advance_approval_state(block_entry,
                           candidate_hash,
                           candidate_entry,
                           approval::LocalApproval{
                               .validator_ix = validator_index,
                               .validator_sig = *sig,
                           });

    import_and_circulate_approval(
        std::nullopt,
        network::IndirectSignedApprovalVote{
            .payload =
                {
                    .payload =
                        network::ApprovalVote{
                            .block_hash = block_hash,
                            .candidate_index = *candidate_index,
                        },
                    .ix = validator_index,
                },
            .signature = std::move(*sig),
        });

    /// TODO(iceseer): store state for the dispute
  }

  std::optional<ValidatorSignature> ApprovalDistribution::sign_approval(
      crypto::Sr25519PublicKey const &pubkey,
      SessionIndex session_index,
      CandidateHash const &candidate_hash) {
    auto key_pair =
        keystore_->findSr25519Keypair(crypto::KEY_TYPE_PARA, pubkey);
    if (key_pair.has_error()) {
      logger_->warn("No key pair in store for {}", pubkey);
      return std::nullopt;
    }
    static std::array<uint8_t, 4ull> kMagic{'A', 'P', 'P', 'R'};
    auto d = std::make_tuple(kMagic, candidate_hash, session_index);
    auto payload = scale::encode(std::move(d)).value();

    if (auto res = crypto_provider_->sign(key_pair.value(), payload);
        res.has_value()) {
      return res.value();
    } else {
      logger_->warn("Unable to sign with {}", pubkey);
      return std::nullopt;
    }
  }

  void ApprovalDistribution::runLaunchApproval(
      CandidateHash const &candidate_hash,
      approval::IndirectAssignmentCert const &indirect_cert,
      DelayTranche assignment_tranche,
      RelayHash const &relay_block_hash,
      CandidateIndex candidate_index,
      SessionIndex session,
      network::CandidateReceipt const &candidate,
      GroupIndex backing_group) {
    /// TODO(iceseer): don't launch approval work if the node is syncing.
    auto const &block_hash = indirect_cert.block_hash;
    auto const validator_index = indirect_cert.validator;

    import_and_circulate_assignment(
        std::nullopt, indirect_cert, candidate_index);
    /// TODO(iceseer): make cache by 'candidate_hash'
    launch_approval(relay_block_hash,
                    candidate_hash,
                    session,
                    candidate,
                    validator_index,
                    block_hash,
                    backing_group);
  }

  void ApprovalDistribution::schedule_wakeup_action(
      ApprovalEntry const &approval_entry,
      Hash const &block_hash,
      BlockNumber block_number,
      CandidateHash const &candidate_hash,
      Tick block_tick,
      Tick tick_now,
      approval::RequiredTranches const &required_tranches) {
    std::optional<Tick> tick{};
    if (!approval_entry.approved) {
      tick = visit_in_place(
          required_tranches,
          [](approval::AllRequiredTranche const &) -> std::optional<Tick> {
            return std::nullopt;
          },
          [&tick_now](approval::ExactRequiredTranche const &e) {
            auto filter = [](Tick const &t, Tick const &ref) {
              auto const v = t + kApprovalDelay;
              return ((v > ref) ? std::optional<Tick>{v}
                                : std::optional<Tick>{});
            };
            return approval::min_or_some(
                e.next_no_show,
                (e.last_assignment_tick
                     ? filter(*e.last_assignment_tick, tick_now)
                     : std::optional<Tick>{}));
          },
          [&](approval::PendingRequiredTranche const &e) {
            std::optional<DelayTranche> next_announced{};
            for (auto const &t : approval_entry.tranches) {
              if (t.tranche > e.considered) {
                next_announced = t.tranche;
                break;
              }
            }
            std::optional<DelayTranche> our_untriggered{};
            if (approval_entry.our_assignment) {
              auto &t = *approval_entry.our_assignment;
              if (!t.triggered && t.tranche > e.considered) {
                our_untriggered = t.tranche;
              }
            }

            auto next_non_empty_tranche =
                approval::min_or_some(next_announced, our_untriggered);
            if (next_non_empty_tranche) {
              *next_non_empty_tranche += (block_tick + e.clock_drift);
            }

            return approval::min_or_some(next_non_empty_tranche,
                                         e.next_no_show);
          });
    }

    if (tick) {
      runScheduleWakeup(block_hash, block_number, candidate_hash, *tick);
    } else {
      logger_->trace(
          "No wakeup. Block hash {}, candidate hash {}, block number {}, tick "
          "{}",
          block_hash,
          candidate_hash,
          block_number,
          tick);
    }
  }

  void ApprovalDistribution::notifyApproved(Hash const &block_hash) {
    /// Action::NoteApprovedInChainSelection => ChainSelectionMessage::Approved
    if (auto result = block_tree_->markAsParachainDataBlock(block_hash);
        result.has_error()) {
      logger_->warn(
          "Adjust weight for block with parachain data failed.(block hash={}, "
          "error={})",
          block_hash,
          result.error().message());
    }
  }

  void ApprovalDistribution::advance_approval_state(
      BlockEntry &block_entry,
      CandidateHash const &candidate_hash,
      CandidateEntry &candidate_entry,
      approval::ApprovalStateTransition transition) {
    auto const validator_index = approval::validator_index(transition);
    std::optional<bool> const already_approved_by =
        validator_index ? candidate_entry.mark_approval(*validator_index)
                        : std::optional<bool>{};
    auto const candidate_approved_in_block =
        block_entry.is_candidate_approved(candidate_hash);

    if (!approval::is_local_approval(transition)) {
      if (candidate_approved_in_block) {
        return;
      }
    }

    auto const &block_hash = block_entry.block_hash;
    auto const block_number = block_entry.block_number;
    auto const tick_now = ::tickNow();

    logger_->info(
        "Advance approval state.(candidate {}, block {}, "
        "validator {})",
        candidate_hash,
        block_hash,
        validator_index);

    auto result = approval_status(block_entry, candidate_entry);
    if (!result) {
      logger_->warn(
          "No approval entry for approval on block: candidate {}, block {}, "
          "validator {}",
          candidate_hash,
          block_hash,
          validator_index);
      return;
    }

    std::optional<std::pair<bool, approval::ApprovalStatus>> _{};
    std::optional<std::reference_wrapper<ApprovalEntry>> ae{};
    {
      auto &[approval_entry, status] = *result;
      auto const check = checkApproval(
          candidate_entry, approval_entry, status.required_tranches);
      auto const is_approved = approval::is_approved(
          check, math::sat_sub_unsigned(tick_now, kApprovalDelay));

      if (is_approved) {
        logger_->info("Candidate approved: candidate {}, block {}",
                      candidate_hash,
                      block_hash);
        auto const was_block_approved = block_entry.is_fully_approved();
        block_entry.mark_approved_by_hash(candidate_hash);
        auto const is_block_approved = block_entry.is_fully_approved();

        if (is_block_approved && !was_block_approved) {
          notifyApproved(block_hash);
        }
        /// TODO(iceseer): store in database if needed
      }
      _ = std::make_pair(is_approved, std::move(status));
      ae = approval_entry;
    }

    BOOST_ASSERT(_);
    BOOST_ASSERT(ae);
    auto &[is_approved, status] = *_;
    auto &approval_entry = ae->get();

    auto const was_approved = approval_entry.approved;
    auto const newly_approved = is_approved && !was_approved;
    if (is_approved) {
      approval_entry.approved = true;
    }

    if (auto v{boost::get<approval::LocalApproval>(&transition)}) {
      approval_entry.our_approval_sig = v->validator_sig;
    }

    schedule_wakeup_action(approval_entry,
                           block_hash,
                           block_number,
                           candidate_hash,
                           status.block_tick,
                           tick_now,
                           status.required_tranches);
    if (approval::is_local_approval(transition) || newly_approved
        || (already_approved_by && !*already_approved_by)) {
      /// TODO(iceseer): store in database if needed
      BOOST_ASSERT(storedCandidateEntries().get(candidate_hash)->get()
                   == candidate_entry);
    }
  }

  void ApprovalDistribution::scheduleTranche(
      primitives::BlockHash const &head, BlockImportedCandidates &&candidate) {
    /// this_thread_ context execution.
    BOOST_ASSERT(this_context_->get_executor().running_in_this_thread());
    SL_TRACE(logger_,
             "Imported new block {}:{} with candidates count {}",
             candidate.block_number,
             candidate.block_hash,
             candidate.imported_candidates.size());

    for (auto const &[c_hash, c_entry] : candidate.imported_candidates) {
      auto const &block_assignments = c_entry.block_assignments.at(head);
      if (block_assignments.our_assignment) {
        auto const our_tranche = block_assignments.our_assignment->tranche;
        auto const tick = our_tranche + candidate.block_tick;
        SL_TRACE(logger_,
                 "Scheduling first wakeup for block {}, tranche {} "
                 "after {}.",
                 candidate.block_hash,
                 our_tranche,
                 tick);

        // Our first wakeup will just be the tranche of our
        // assignment, if any. This will likely be superseded by
        // incoming assignments and approvals which trigger
        // rescheduling.
        runScheduleWakeup(
            candidate.block_hash, candidate.block_number, c_hash, tick);
      }
    }
  }

  void ApprovalDistribution::runScheduleWakeup(
      primitives::BlockHash const &block_hash,
      primitives::BlockNumber block_number,
      CandidateHash const &candidate_hash,
      Tick tick) {
    logger_->info(
        "Scheduling wakeup. Block hash {}, candidate hash {}, block number {}, "
        "tick {}",
        block_hash,
        candidate_hash,
        block_number,
        tick);

    auto t = std::make_unique<clock::BasicWaitableTimer>(this_context_);
    t->expiresAfter(std::chrono::milliseconds(tick));
    t->asyncWait([wself{weak_from_this()},
                  id{uintptr_t(t.get())},
                  block_hash,
                  block_number,
                  candidate_hash](auto &&ec) {
      if (auto self = wself.lock()) {
        self->active_tranches_.erase(id);
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
    active_tranches_[uintptr_t(t.get())] = std::move(t);
  }

  void ApprovalDistribution::handleTranche(
      const primitives::BlockHash &block_hash,
      primitives::BlockNumber block_number,
      const CandidateHash &candidate_hash) {
    BOOST_ASSERT(this_context_->get_executor().running_in_this_thread());

    auto opt_block_entry = storedBlockEntries().get(block_hash);
    auto opt_candidate_entry = storedCandidateEntries().get(candidate_hash);

    if (!opt_block_entry || !opt_candidate_entry) {
      SL_ERROR(logger_, "Block entry or candidate entry not exists.");
      return;
    }

    auto &block_entry = opt_block_entry->get();
    auto &candidate_entry = opt_candidate_entry->get();
    auto &session_info = opt_block_entry->get().session_info;

    auto const block_tick =
        slotNumberToTick(config_.slot_duration_millis, block_entry.slot);
    auto const no_show_duration = slotNumberToTick(config_.slot_duration_millis,
                                                   session_info.no_show_slots);

    auto const tranche_now =
        ::trancheNow(config_.slot_duration_millis, block_entry.slot);
    SL_TRACE(logger_,
             "Processing wakeup: tranche={}, candidate_hash={}, relay_hash={}",
             tranche_now,
             candidate_hash,
             block_hash);

    auto opt_approval_entry = candidate_entry.approval_entry(block_hash);
    if (!opt_approval_entry) {
      return;
    }

    auto &approval_entry = opt_approval_entry->get();
    auto const tta = tranchesToApprove(approval_entry,
                                       candidate_entry.approvals,
                                       tranche_now,
                                       block_tick,
                                       no_show_duration,
                                       session_info.needed_approvals);
    auto const should_trigger = shouldTriggerAssignment(
        approval_entry, candidate_entry, tta, tranche_now);
    auto const backing_group = approval_entry.backing_group;
    auto candidate_receipt = candidate_entry.candidate;

    ApprovalEntry::MaybeCert maybe_cert{};
    if (should_trigger) {
      maybe_cert = approval_entry.trigger_our_assignment(tickNow());
      BOOST_ASSERT(storedCandidateEntries().get(candidate_hash)->get()
                   == candidate_entry);
    }

    if (maybe_cert) {
      auto const &[cert, val_index, tranche] = *maybe_cert;
      approval::IndirectAssignmentCert indirect_cert{
          .block_hash = block_hash,
          .validator = val_index,
          .cert = cert.get(),
      };

      if (auto i = block_entry.candidateIxByHash(candidate_hash)) {
        runLaunchApproval(candidate_hash,
                          indirect_cert,
                          tranche,
                          block_hash,
                          CandidateIndex(*i),
                          block_entry.session,
                          candidate_receipt,
                          backing_group);
      }
    }

    advance_approval_state(block_entry,
                           candidate_hash,
                           candidate_entry,
                           approval::WakeupProcessed{});
  }

}  // namespace kagome::parachain
