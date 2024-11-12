/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <optional>

#include <fmt/std.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <future>
#include <libp2p/basic/scheduler/asio_scheduler_backend.hpp>
#include <libp2p/basic/scheduler/scheduler_impl.hpp>
#include <libp2p/common/shared_fn.hpp>

#include "common/main_thread_pool.hpp"
#include "common/visitor.hpp"
#include "common/worker_thread_pool.hpp"
#include "consensus/babe/babe_config_repository.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "crypto/hasher.hpp"
#include "crypto/key_store.hpp"
#include "crypto/sr25519_provider.hpp"
#include "network/impl/protocols/parachain.hpp"
#include "network/peer_manager.hpp"
#include "network/router.hpp"
#include "parachain/approval/approval.hpp"
#include "parachain/approval/approval_distribution.hpp"
#include "parachain/approval/approval_distribution_error.hpp"
#include "parachain/approval/approval_thread_pool.hpp"
#include "parachain/approval/state.hpp"
#include "primitives/math.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"
#include "utils/pool_handler_ready_make.hpp"
#include "utils/weak_macro.hpp"

static constexpr size_t kMaxAssignmentBatchSize = 200ull;
static constexpr size_t kMaxApprovalBatchSize = 300ull;
static constexpr size_t kMaxBitfieldSize = 500ull;

static constexpr uint64_t kTickDurationMs = 500ull;
static constexpr kagome::network::Tick kApprovalDelay = 2ull;
static constexpr kagome::network::Tick kTickTooFarInFuture =
    20ull;  // 10 seconds.

namespace {

  /// assumes `slot_duration_millis` evenly divided by tick duration.
  kagome::network::Tick slotNumberToTick(uint64_t slot_duration_millis,
                                         kagome::consensus::SlotNumber slot) {
    const auto ticks_per_slot = slot_duration_millis / kTickDurationMs;
    return slot * ticks_per_slot;
  }

  uint64_t msNow() {
    return uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count());
  }

  kagome::network::Tick tickNow() {
    return msNow() / kTickDurationMs;
  }

  kagome::parachain::approval::DelayTranche trancheNow(
      uint64_t slot_duration_millis, kagome::consensus::SlotNumber base_slot) {
    return static_cast<kagome::parachain::approval::DelayTranche>(
        kagome::math::sat_sub_unsigned(
            tickNow(), slotNumberToTick(slot_duration_millis, base_slot)));
  }

  bool isInBackingGroup(
      const std::vector<std::vector<kagome::parachain::ValidatorIndex>>
          &validator_groups,
      kagome::parachain::ValidatorIndex validator,
      kagome::parachain::GroupIndex group) {
    if (group < validator_groups.size()) {
      for (const auto &i : validator_groups[group]) {
        if (i == validator) {
          return true;
        }
      }
    }
    return false;
  }

  void computeVrfModuloAssignments_v2(
      std::span<const uint8_t, kagome::crypto::constants::sr25519::KEYPAIR_SIZE>
          assignments_key,
      const kagome::runtime::SessionInfo &config,
      const RelayVRFStory &relay_vrf_story,
      const std::vector<kagome::parachain::CoreIndex> &leaving_cores,
      kagome::parachain::ValidatorIndex validator_index,
      std::unordered_map<kagome::parachain::CoreIndex,
                         kagome::parachain::ApprovalDistribution::OurAssignment>
          &assignments) {
    using namespace kagome::parachain;  // NOLINT(google-build-using-namespace)
    using namespace kagome;             // NOLINT(google-build-using-namespace)

    VRFCOutput cert_output;
    VRFCProof cert_proof;
    uint32_t *cores;        // NOLINT(cppcoreguidelines-init-variables)
    uint64_t cores_out_sz;  // NOLINT(cppcoreguidelines-init-variables)

    if (sr25519_relay_vrf_modulo_assignments_cert_v2(
            assignments_key.data(),
            config.relay_vrf_modulo_samples,
            config.n_cores,
            &relay_vrf_story,
            leaving_cores.data(),
            leaving_cores.size(),
            &cert_output,
            &cert_proof,
            &cores,
            &cores_out_sz)) {
      ::scale::BitVec assignment_bitfield;
      for (size_t ix = 0; ix < cores_out_sz; ++ix) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        const auto ci = cores[ix];
        if (ci >= assignment_bitfield.bits.size()) {
          assignment_bitfield.bits.resize(ci + 1);
        }
        assignment_bitfield.bits[ci] = true;
      }

      crypto::VRFPreOutput o;
      std::copy_n(std::make_move_iterator(cert_output.data),
                  crypto::constants::sr25519::vrf::OUTPUT_SIZE,
                  o.begin());

      crypto::VRFProof p;
      std::copy_n(std::make_move_iterator(cert_proof.data),
                  crypto::constants::sr25519::vrf::PROOF_SIZE,
                  p.begin());

      ApprovalDistribution::OurAssignment assignment{
          .cert =
              approval::AssignmentCertV2{
                  .kind =
                      approval::RelayVRFModuloCompact{
                          .core_bitfield = assignment_bitfield,
                      },
                  .vrf = crypto::VRFOutput{.output = o, .proof = p},
              },
          .tranche = 0ul,
          .validator_index = validator_index,
          .triggered = false};

      for (size_t ix = 0; ix < cores_out_sz; ++ix) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        const auto core_index = cores[ix];
        assignments.emplace(core_index, assignment);
      }

      sr25519_clear_assigned_cores_v2(cores, cores_out_sz);
    }
  }

  void computeVrfModuloAssignments_v1(
      std::span<const uint8_t, kagome::crypto::constants::sr25519::KEYPAIR_SIZE>
          keypair_buf,
      const kagome::runtime::SessionInfo &config,
      const RelayVRFStory &relay_vrf_story,
      const std::vector<kagome::parachain::CoreIndex> &lc,
      kagome::parachain::ValidatorIndex validator_ix,
      std::unordered_map<kagome::parachain::CoreIndex,
                         kagome::parachain::ApprovalDistribution::OurAssignment>
          &assignments) {
    using namespace kagome::parachain;  // NOLINT(google-build-using-namespace)
    using namespace kagome;             // NOLINT(google-build-using-namespace)

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
        if (assignments.contains(core)) {
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
                    approval::AssignmentCertV2::from(approval::AssignmentCert{
                        .kind = approval::RelayVRFModulo{.sample = rvm_sample},
                        .vrf = crypto::VRFOutput{.output = o, .proof = p}}),
                .tranche = 0ul,
                .validator_index = validator_ix,
                .triggered = false});
      }
    }
  }

  void computeVrfDelayAssignments(
      std::span<const uint8_t, kagome::crypto::constants::sr25519::KEYPAIR_SIZE>
          keypair_buf,
      const kagome::runtime::SessionInfo &config,
      const RelayVRFStory &relay_vrf_story,
      const std::vector<kagome::parachain::CoreIndex> &lc,
      kagome::parachain::ValidatorIndex validator_ix,
      std::unordered_map<kagome::parachain::CoreIndex,
                         kagome::parachain::ApprovalDistribution::OurAssignment>
          &assignments) {
    using namespace kagome::parachain;  // NOLINT(google-build-using-namespace)
    using namespace kagome;             // NOLINT(google-build-using-namespace)

    VRFCOutput cert_output;
    VRFCProof cert_proof;
    uint32_t tranche = {};
    for (const auto &core : lc) {
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
                    approval::AssignmentCertV2{
                        .kind = approval::RelayVRFDelay{.core_index = core},
                        .vrf = crypto::VRFOutput{.output = o, .proof = p}},
                .tranche = tranche,
                .validator_index = validator_ix,
                .triggered = false});
      }
    }
  }

  /// Determine the amount of tranches of assignments needed to determine
  /// approval of a candidate.
  kagome::parachain::approval::RequiredTranches tranchesToApprove(
      const kagome::parachain::ApprovalDistribution::ApprovalEntry
          &approval_entry,
      const scale::BitVec &approvals,
      kagome::network::DelayTranche tranche_now,
      kagome::parachain::Tick block_tick,
      kagome::parachain::Tick no_show_duration,
      size_t needed_approvals) {
    const auto tick_now = tranche_now + block_tick;
    const auto n_validators = approval_entry.n_validators();

    std::optional<kagome::parachain::approval::State> state(needed_approvals);
    const auto &tranches = approval_entry.tranches;

    auto it = [&](uint32_t tranche)
        -> std::optional<kagome::parachain::approval::RequiredTranches> {
      auto s = *state;
      const auto clock_drift = s.depth * no_show_duration;
      const auto drifted_tick_now =
          kagome::math::sat_sub_unsigned(tick_now, clock_drift);
      const auto drifted_tranche_now =
          kagome::math::sat_sub_unsigned(drifted_tick_now, block_tick);

      if (tranche > drifted_tranche_now) {
        return std::nullopt;
      }

      size_t n_assignments = 0ull;
      std::optional<kagome::parachain::Tick> last_assignment_tick{};
      size_t no_shows = 0ull;
      std::optional<uint64_t> next_no_show{};

      if (auto i = std::ranges::lower_bound(
              tranches,
              tranche,
              std::less<>(),
              &kagome::parachain::ApprovalDistribution::TrancheEntry::tranche);
          i != tranches.end() && i->tranche == tranche) {
        for (const auto &[v_index, t] : i->assignments) {
          if (v_index < n_validators) {
            ++n_assignments;
          }
          last_assignment_tick =
              std::max(t,
                       last_assignment_tick ? *last_assignment_tick
                                            : kagome::parachain::Tick{0ull});
          const auto no_show_at = kagome::math::sat_sub_unsigned(
                                      std::max(t, block_tick), clock_drift)
                                + no_show_duration;
          if (v_index < approvals.bits.size()) {
            const auto has_approved = approvals.bits.at(v_index);
            const auto is_no_show =
                !has_approved && no_show_at <= drifted_tick_now;
            if (!is_no_show && !has_approved) {
              next_no_show = kagome::parachain::approval::min_or_some(
                  next_no_show, std::make_optional(no_show_at + clock_drift));
            }
            if (is_no_show) {
              ++no_shows;
            }
          }
        }
      }

      s = s.advance(
          n_assignments, no_shows, next_no_show, last_assignment_tick);
      auto output =
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

  scale::BitVec &filter(scale::BitVec &lh, const scale::BitVec &rh) {
    BOOST_ASSERT(lh.bits.size() == rh.bits.size());
    for (size_t ix = 0; ix < lh.bits.size(); ++ix) {
      lh.bits[ix] = (lh.bits[ix] && rh.bits[ix]);
    }
    return lh;
  }

  kagome::parachain::approval::Check checkApproval(
      const kagome::parachain::ApprovalDistribution::CandidateEntry &candidate,
      const kagome::parachain::ApprovalDistribution::ApprovalEntry &approval,
      const kagome::parachain::approval::RequiredTranches &required) {
    const auto &approvals = candidate.approvals;
    if (3 * kagome::parachain::approval::count_ones(approvals)
        > approvals.bits.size()) {
      return kagome::parachain::approval::ApprovedOneThird{};
    }

    if (kagome::is_type<kagome::parachain::approval::PendingRequiredTranche>(
            required)) {
      return kagome::parachain::approval::Unapproved{};
    }
    if (kagome::is_type<kagome::parachain::approval::AllRequiredTranche>(
            required)) {
      return kagome::parachain::approval::Unapproved{};
    }
    if (auto exact{
            boost::get<kagome::parachain::approval::ExactRequiredTranche>(
                &required)}) {
      auto assigned_mask = approval.assignments_up_to(exact->needed);
      kagome::log::Logger logger_ =
          kagome::log::createLogger("ApprovalDistribution", "parachain");
      SL_TRACE(logger_,
               "assigned_mask=[{}] approvals=[{}] (candidate={})",
               fmt::join(assigned_mask.bits, ","),
               fmt::join(approvals.bits, ","),
               candidate.candidate.getHash());
      const auto n_assigned =
          kagome::parachain::approval::count_ones(assigned_mask);
      filter(assigned_mask, approvals);
      const auto n_approved =
          kagome::parachain::approval::count_ones(assigned_mask);
      if (n_approved + exact->tolerated_missing >= n_assigned) {
        return std::make_pair(exact->tolerated_missing,
                              exact->last_assignment_tick);
      }
      return kagome::parachain::approval::Unapproved{};
    }
    UNREACHABLE;
  }

  bool shouldTriggerAssignment(
      const kagome::parachain::ApprovalDistribution::ApprovalEntry
          &approval_entry,
      const kagome::parachain::ApprovalDistribution::CandidateEntry
          &candidate_entry,
      const kagome::parachain::approval::RequiredTranches &required_tranches,
      const kagome::network::DelayTranche tranche_now) {
    if (!approval_entry.our_assignment) {
      return false;
    }
    if (approval_entry.our_assignment->triggered) {
      return false;
    }
    if (approval_entry.our_assignment->tranche == 0) {
      return true;
    }
    if (kagome::is_type<kagome::parachain::approval::AllRequiredTranche>(
            required_tranches)) {
      return !kagome::parachain::approval::is_approved(
          checkApproval(candidate_entry,
                        approval_entry,
                        kagome::parachain::approval::AllRequiredTranche{}),
          std::numeric_limits<kagome::network::Tick>::max());
    }
    if (auto pending = kagome::if_type<
            const kagome::parachain::approval::PendingRequiredTranche>(
            required_tranches)) {
      const auto drifted_tranche_now = kagome::math::sat_sub_unsigned(
          tranche_now,
          kagome::network::DelayTranche(pending->get().clock_drift));
      return approval_entry.our_assignment->tranche
              <= pending->get().maximum_broadcast
          && approval_entry.our_assignment->tranche <= drifted_tranche_now;
    }
    if (kagome::is_type<kagome::parachain::approval::ExactRequiredTranche>(
            required_tranches)) {
      return false;
    }
    UNREACHABLE;
  }

  outcome::result<kagome::network::DelayTranche> checkAssignmentCert(
      const scale::BitVec &claimed_core_indices,
      kagome::network::ValidatorIndex validator_index,
      const kagome::runtime::SessionInfo &config,
      const RelayVRFStory &relay_vrf_story,
      const kagome::parachain::approval::AssignmentCertV2 &assignment,
      const std::vector<kagome::network::GroupIndex> &backing_groups) {
    using namespace kagome;  // NOLINT(google-build-using-namespace)
    using parachain::ApprovalDistributionError;

    if (validator_index >= config.assignment_keys.size()) {
      return ApprovalDistributionError::VALIDATOR_INDEX_OUT_OF_BOUNDS;
    }

    const auto &validator_public = config.assignment_keys[validator_index];
    //    OUTCOME_TRY(pk, network::ValidatorId::fromSpan(validator_public));

    if (kagome::parachain::approval::count_ones(claimed_core_indices) == 0
        || kagome::parachain::approval::count_ones(claimed_core_indices)
               != backing_groups.size()) {
      return ApprovalDistributionError::CORE_INDEX_OUT_OF_BOUNDS;
    }

    // Check that the validator was not part of the backing group
    // and not already assigned.
    for (size_t claimed_core = 0, b_i = 0;
         claimed_core < claimed_core_indices.bits.size();
         ++claimed_core) {
      if (!claimed_core_indices.bits[claimed_core]) {
        continue;
      }

      const auto backing_group = backing_groups[b_i++];
      if (claimed_core >= config.n_cores) {
        return ApprovalDistributionError::CORE_INDEX_OUT_OF_BOUNDS;
      }

      const auto is_in_backing = isInBackingGroup(
          config.validator_groups, validator_index, backing_group);
      if (is_in_backing) {
        return ApprovalDistributionError::IS_IN_BACKING_GROUP;
      }
    }

    const auto &vrf_output = assignment.vrf.output;
    const auto &vrf_proof = assignment.vrf.proof;
    const auto first_claimed_core_index = [&]() {
      for (uint32_t i = 0; i < claimed_core_indices.bits.size(); ++i) {
        if (claimed_core_indices.bits[i]) {
          return i;
        }
      }
      throw std::runtime_error(
          "Unexpected bitslice content. No `true` found, but expect.");
    }();

    return visit_in_place(
        assignment.kind,
        [&](const parachain::approval::RelayVRFModuloCompact &obj)
            -> outcome::result<kagome::network::DelayTranche> {
          const auto core_bitfield = obj.core_bitfield;
          if (claimed_core_indices != core_bitfield) {
            return ApprovalDistributionError::VRF_MODULO_CORE_INDEX_MISMATCH;
          }

          /// TODO(iceseer): `vrf_verify_extra` check
          /// TODO(iceseer): `relay_vrf_modulo_core`
          return network::DelayTranche(0ull);
        },
        [&](const parachain::approval::RelayVRFModulo &obj)
            -> outcome::result<kagome::network::DelayTranche> {
          const auto sample = obj.sample;
          if (sample >= config.relay_vrf_modulo_samples) {
            return ApprovalDistributionError::SAMPLE_OUT_OF_BOUNDS;
          }
          /// TODO(iceseer): `vrf_verify_extra` check
          /// TODO(iceseer): `relay_vrf_modulo_core`
          return network::DelayTranche(0ull);
        },
        [&](const parachain::approval::RelayVRFDelay &obj)
            -> outcome::result<kagome::network::DelayTranche> {
          const auto core_index = obj.core_index;
          if (parachain::approval::count_ones(claimed_core_indices) != 1) {
            return ApprovalDistributionError::INVALID_ARGUMENTS;
          }

          if (core_index != first_claimed_core_index) {
            return ApprovalDistributionError::VRF_DELAY_CORE_INDEX_MISMATCH;
          }

          // NOLINTNEXTLINE(cppcoreguidelines-init-variables)
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
            return ApprovalDistributionError::VRF_VERIFY_AND_GET_TRANCHE;
          }
          return tranche;
        });
  }

}  // namespace

namespace kagome::parachain {
  constexpr auto kMetricNoShowsTotal =
      "kagome_parachain_approvals_no_shows_total";

  ApprovalDistribution::ApprovalDistribution(
      std::shared_ptr<consensus::babe::BabeConfigRepository> babe_config_repo,
      std::shared_ptr<application::AppStateManager> app_state_manager,
      primitives::events::ChainSubscriptionEnginePtr chain_sub_engine,
      common::WorkerThreadPool &worker_thread_pool,
      std::shared_ptr<runtime::ParachainHost> parachain_host,
      LazySPtr<consensus::SlotsUtil> slots_util,
      std::shared_ptr<crypto::KeyStore> keystore,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<network::PeerView> peer_view,
      std::shared_ptr<ParachainProcessorImpl> parachain_processor,
      std::shared_ptr<crypto::Sr25519Provider> crypto_provider,
      std::shared_ptr<network::PeerManager> pm,
      std::shared_ptr<network::Router> router,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<Pvf> pvf,
      std::shared_ptr<Recovery> recovery,
      ApprovalThreadPool &approval_thread_pool,
      common::MainThreadPool &main_thread_pool,
      LazySPtr<dispute::DisputeCoordinator> dispute_coordinator)
      : approval_thread_handler_{poolHandlerReadyMake(
          this, app_state_manager, approval_thread_pool, logger_)},
        worker_pool_handler_{worker_thread_pool.handler(*app_state_manager)},
        parachain_host_(std::move(parachain_host)),
        slots_util_(slots_util),
        keystore_(std::move(keystore)),
        hasher_(std::move(hasher)),
        config_(ApprovalVotingSubsystem{.slot_duration_millis = 6'000}),
        peer_view_(std::move(peer_view)),
        chain_sub_{std::move(chain_sub_engine)},
        parachain_processor_(std::move(parachain_processor)),
        crypto_provider_(std::move(crypto_provider)),
        pm_(std::move(pm)),
        router_(std::move(router)),
        babe_config_repo_(std::move(babe_config_repo)),
        block_tree_(std::move(block_tree)),
        pvf_(std::move(pvf)),
        recovery_(std::move(recovery)),
        main_pool_handler_{main_thread_pool.handler(*app_state_manager)},
        dispute_coordinator_{dispute_coordinator},
        scheduler_{std::make_shared<libp2p::basic::SchedulerImpl>(
            std::make_shared<libp2p::basic::AsioSchedulerBackend>(
                approval_thread_pool.io_context()),
            libp2p::basic::Scheduler::Config{})},
        metrics_registry_{metrics::createRegistry()} {
    BOOST_ASSERT(parachain_host_);
    BOOST_ASSERT(keystore_);
    BOOST_ASSERT(peer_view_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(parachain_processor_);
    BOOST_ASSERT(crypto_provider_);
    BOOST_ASSERT(pm_);
    BOOST_ASSERT(router_);
    BOOST_ASSERT(babe_config_repo_);
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(pvf_);
    BOOST_ASSERT(recovery_);
    BOOST_ASSERT(main_pool_handler_);
    BOOST_ASSERT(worker_pool_handler_);
    BOOST_ASSERT(approval_thread_handler_);

    metrics_registry_->registerCounterFamily(
        kMetricNoShowsTotal,
        "Number of assignments which became no-shows in the approval voting "
        "subsystem");
    metric_no_shows_total_ =
        metrics_registry_->registerCounterMetric(kMetricNoShowsTotal);
  }

  bool ApprovalDistribution::tryStart() {
    my_view_sub_ = primitives::events::subscribe(
        peer_view_->getMyViewObservable(),
        network::PeerView::EventType::kViewUpdated,
        [WEAK_SELF](const network::ExView &event) {
          WEAK_LOCK(self);
          self->on_active_leaves_update(event);
        });

    remote_view_sub_ = primitives::events::subscribe(
        peer_view_->getRemoteViewObservable(),
        network::PeerView::EventType::kViewUpdated,
        [WEAK_SELF](const libp2p::peer::PeerId &peer_id,
                    const network::View &view) {
          WEAK_LOCK(self);
          self->store_remote_view(peer_id, view);
        });

    chain_sub_.onDeactivate(
        [WEAK_SELF](
            const primitives::events::RemoveAfterFinalizationParams &event) {
          WEAK_LOCK(self);
          self->clearCaches(event);
        });

    /// TODO(iceseer): clear `known_by` when peer disconnected

    return true;
  }

  bool ApprovalDistribution::BlockEntry::operator==(const BlockEntry &l) const {
    return block_hash == l.block_hash && parent_hash == l.parent_hash
        && block_number == l.block_number && session == l.session
        && slot == l.slot && candidates == l.candidates
        && approved_bitfield == l.approved_bitfield
        && distributed_assignments == l.distributed_assignments
        && children == l.children;
  }

  void ApprovalDistribution::store_remote_view(
      const libp2p::peer::PeerId &peer_id, const network::View &view) {
    REINVOKE(*approval_thread_handler_, store_remote_view, peer_id, view);

    primitives::BlockNumber old_finalized_number{0ull};
    if (auto it = peer_views_.find(peer_id); it != peer_views_.end()) {
      old_finalized_number = it->second.finalized_number_;
    }

    for (primitives::BlockNumber bn = old_finalized_number;
         bn <= view.finalized_number_;
         ++bn) {
      if (auto it = blocks_by_number_.find(bn); it != blocks_by_number_.end()) {
        for (const auto &bh : it->second) {
          if (auto opt_entry = storedDistribBlockEntries().get(bh)) {
            opt_entry->get().known_by.erase(peer_id);
          }
        }
      }
    }

    unify_with_peer(storedDistribBlockEntries(), peer_id, view, false);
    peer_views_[peer_id] = view;
  }

  void ApprovalDistribution::clearCaches(
      const primitives::events::RemoveAfterFinalizationParams &event) {
    REINVOKE(*approval_thread_handler_, clearCaches, event);

    approvals_cache_.exclusiveAccess([&](auto &approvals_cache) {
      for (const auto &lost : event.removed) {
        SL_TRACE(logger_,
                 "Cleaning up stale pending messages.(block hash={})",
                 lost.hash);
        pending_known_.erase(lost.hash);
        active_tranches_.erase(lost.hash);
        approving_context_map_.erase(lost.hash);
        /// TODO(iceseer): `blocks_by_number_` clear on finalization

        if (auto block_entry = storedBlockEntries().get(lost.hash)) {
          for (const auto &candidate : block_entry->get().candidates) {
            recovery_->remove(candidate.second);
            storedCandidateEntries().extract(candidate.second);
            if (auto it_cached = approvals_cache.find(candidate.second);
                it_cached != approvals_cache.end()) {
              ApprovalCache &approval_cache = it_cached->second;
              approval_cache.blocks_.erase(lost.hash);
              if (approval_cache.blocks_.empty()) {
                approvals_cache.erase(it_cached);
              }
            }
          }
          storedBlockEntries().extract(lost.hash);
        }
        storedDistribBlockEntries().extract(lost.hash);
      }
    });
  }

  std::optional<std::pair<ValidatorIndex, crypto::Sr25519Keypair>>
  ApprovalDistribution::findAssignmentKey(
      const std::shared_ptr<crypto::KeyStore> &keystore,
      const runtime::SessionInfo &config) {
    for (size_t ix = 0; ix < config.assignment_keys.size(); ++ix) {
      const auto &pk = config.assignment_keys[ix];
      if (auto res = keystore->sr25519().findKeypair(
              crypto::KeyTypes::ASSIGNMENT,
              crypto::Sr25519PublicKey::fromSpan(pk).value());
          res.has_value()) {
        return std::make_pair((ValidatorIndex)ix, res.value());
      }
    }
    return std::nullopt;
  }

  ApprovalDistribution::AssignmentsList
  ApprovalDistribution::compute_assignments(
      const std::shared_ptr<crypto::KeyStore> &keystore,
      const runtime::SessionInfo &config,
      const RelayVRFStory &relay_vrf_story,
      const CandidateIncludedList &leaving_cores,
      bool enable_v2_assignments,
      log::Logger &logger) {
    if (config.n_cores == 0 || config.assignment_keys.empty()
        || config.validator_groups.empty()) {
      SL_TRACE(logger,
               "Not producing assignments because config is degenerate. "
               "(n_cores={}, assignments_keys={}, validators_groups={})",
               config.n_cores,
               config.assignment_keys.size(),
               config.validator_groups.size());
      return {};
    }

    std::optional<std::pair<ValidatorIndex, crypto::Sr25519Keypair>> found_key =
        findAssignmentKey(keystore, config);
    if (!found_key) {
      return {};
    }

    const auto &[index, assignments_key] = *found_key;
    std::vector<kagome::parachain::CoreIndex> lc;
    for (const auto &[c_hash, core, g] : leaving_cores) {
      if (!isInBackingGroup(config.validator_groups, index, g)) {
        lc.emplace_back(core);
      }
    }

    SL_TRACE(logger,
             "Assigning to candidates from different backing groups. "
             "(assignable_cores={})",
             lc.size());

    if (lc.empty()) {
      return {};
    }

    common::Blob<crypto::constants::sr25519::KEYPAIR_SIZE> keypair_buf{};
    crypto::SecureCleanGuard g{keypair_buf};
    std::ranges::copy(assignments_key.secret_key.unsafeBytes(),
                      keypair_buf.begin());
    std::ranges::copy(assignments_key.public_key,
                      keypair_buf.begin() + crypto::Sr25519SecretKey::size());

    std::unordered_map<CoreIndex, ApprovalDistribution::OurAssignment>
        assignments;
    if (enable_v2_assignments) {
      computeVrfModuloAssignments_v2(
          keypair_buf, config, relay_vrf_story, lc, index, assignments);
    } else {
      computeVrfModuloAssignments_v1(
          keypair_buf, config, relay_vrf_story, lc, index, assignments);
    }
    computeVrfDelayAssignments(
        keypair_buf, config, relay_vrf_story, lc, index, assignments);

    return assignments;
  }

  ApprovalDistribution::DistribApprovalEntry &
  ApprovalDistribution::DistribBlockEntry::insert_approval_entry(
      ApprovalDistribution::DistribApprovalEntry &&entry) {
    std::ignore = approval::iter_ones(
        entry.assignment_claimed_candidates,
        [&](const auto claimed_candidate_index) -> outcome::result<void> {
          if (claimed_candidate_index >= candidates.size()) {
            throw std::runtime_error(
                fmt::format("Missing candidate entry on "
                            "`import_and_circulate_assignment`. (hash={}, "
                            "claimed_candidate_index={})",
                            entry.assignment.block_hash,
                            claimed_candidate_index));
          }

          auto &candidate_entry = candidates[claimed_candidate_index];
          std::ignore = candidate_entry.assignments.emplace(
              entry.validator_index, entry.assignment_claimed_candidates);
          return outcome::success();
        });

    auto [it, _] = approval_entries.emplace(
        std::make_pair(entry.validator_index,
                       entry.assignment_claimed_candidates),
        entry);
    return it->second;
  }

  bool ApprovalDistribution::DistribApprovalEntry::includes_approval_candidates(
      const approval::IndirectSignedApprovalVoteV2 &approval) const {
    return approval::iter_ones(
               getPayload(approval).candidate_indices,
               [&](const auto candidate_index) -> outcome::result<void> {
                 if (candidate_index < assignment_claimed_candidates.bits.size()
                     && assignment_claimed_candidates.bits[candidate_index]) {
                   return ApprovalDistributionError::BIT_FOUND;
                 }
                 return outcome::success();
               })
        .has_error();
  }

  outcome::result<void>
  ApprovalDistribution::DistribApprovalEntry::note_approval(
      const approval::IndirectSignedApprovalVoteV2 &approval_val) {
    const auto &approval = getPayload(approval_val);
    if (validator_index != approval_val.payload.ix) {
      return ApprovalDistributionError::VALIDATOR_INDEX_OUT_OF_BOUNDS;
    }

    if (!includes_approval_candidates(approval_val)) {
      return ApprovalDistributionError::CANDIDATE_INDEX_OUT_OF_BOUNDS;
    }

    if (approvals.contains(approval.candidate_indices)) {
      return ApprovalDistributionError::DUPLICATE_APPROVAL;
    }

    approvals.insert_or_assign(approval.candidate_indices, approval_val);
    return outcome::success();
  }

  std::vector<approval::IndirectSignedApprovalVoteV2>
  ApprovalDistribution::DistribBlockEntry::approval_votes(
      CandidateIndex candidate_index) const {
    std::unordered_map<DistribApprovalEntryKey,
                       approval::IndirectSignedApprovalVoteV2,
                       ApprovalEntryHash>
        result;
    if (auto candidate_entry = utils::get(candidates, candidate_index)) {
      for (const auto &[validator, assignment_bitfield] :
           candidate_entry->get().assignments) {
        if (auto approval_entry =
                utils::get(approval_entries,
                           std::make_pair(validator, assignment_bitfield))) {
          for (const auto &[approved_candidates, vote] :
               approval_entry->get().approvals) {
            if (candidate_index < approved_candidates.bits.size()
                && approved_candidates.bits[candidate_index]) {
              result[std::make_pair(approval_entry->get().validator_index,
                                    approved_candidates)] = vote;
            }
          }
        }
      }
    }

    std::vector<approval::IndirectSignedApprovalVoteV2> out;
    out.reserve(result.size());

    std::ranges::transform(result, std::back_inserter(out), [](const auto &it) {
      return it.second;
    });
    return out;
  }

  outcome::result<std::pair<grid::RequiredRouting,
                            std::unordered_set<libp2p::peer::PeerId>>>
  ApprovalDistribution::DistribBlockEntry::note_approval(
      const approval::IndirectSignedApprovalVoteV2 &approval_value) {
    const approval::IndirectApprovalVoteV2 &approval =
        getPayload(approval_value);

    std::optional<grid::RequiredRouting> required_routing;
    std::unordered_set<libp2p::peer::PeerId> peers_randomly_routed_to;

    if (candidates.size() < approval.candidate_indices.bits.size()) {
      return ApprovalDistributionError::CANDIDATE_INDEX_OUT_OF_BOUNDS;
    }

    std::unordered_set<scale::BitVec> covered_assignments_bitfields;
    std::ignore = approval::iter_ones(
        approval.candidate_indices,
        [&](const auto candidate_index) -> outcome::result<void> {
          if (candidate_index < candidates.size()) {
            const auto &candidate_entry = candidates[candidate_index];
            if (auto it = utils::get(candidate_entry.assignments,
                                     approval_value.payload.ix)) {
              covered_assignments_bitfields.insert(it->get());
            }
          }
          return outcome::success();
        });

    for (const auto &assignment_bitfield : covered_assignments_bitfields) {
      if (auto it = utils::get(
              approval_entries,
              std::make_pair(approval_value.payload.ix, assignment_bitfield))) {
        auto &approval_entry = it->get();
        OUTCOME_TRY(approval_entry.note_approval(approval_value));

        peers_randomly_routed_to.insert(
            approval_entry.routing_info.peers_randomly_routed.begin(),
            approval_entry.routing_info.peers_randomly_routed.end());
        if (required_routing) {
          if (*required_routing
              != approval_entry.routing_info.required_routing) {
            return ApprovalDistributionError::
                ASSIGNMENTS_FOLLOWED_DIFFERENT_PATH;
          }
        } else {
          required_routing = approval_entry.routing_info.required_routing;
        }
      }
    }

    if (required_routing) {
      return std::make_pair(*required_routing,
                            std::move(peers_randomly_routed_to));
    }
    return ApprovalDistributionError::UNKNOWN_ASSIGNMENT;
  }

  std::optional<scale::BitVec>
  ApprovalDistribution::get_assignment_core_indices(
      const approval::AssignmentCertKindV2 &assignment,
      const CandidateHash &candidate_hash,
      const BlockEntry &block_entry) {
    return visit_in_place(
        assignment,
        [&](const kagome::parachain::approval::RelayVRFModuloCompact &value)
            -> std::optional<scale::BitVec> { return value.core_bitfield; },
        [&](const kagome::parachain::approval::RelayVRFModulo &value)
            -> std::optional<scale::BitVec> {
          for (const auto &[core_index, h] : block_entry.candidates) {
            if (candidate_hash == h) {
              scale::BitVec v;
              v.bits.resize(core_index + 1);
              v.bits[core_index] = true;
              return v;
            }
          }
          return std::nullopt;
        },
        [&](const kagome::parachain::approval::RelayVRFDelay &value)
            -> std::optional<scale::BitVec> {
          scale::BitVec v;
          v.bits.resize(value.core_index + 1);
          v.bits[value.core_index] = true;
          return v;
        });
  }

  std::optional<scale::BitVec> ApprovalDistribution::cores_to_candidate_indices(
      const scale::BitVec &core_indices, const BlockEntry &block_entry) {
    std::vector<uint32_t> candidate_indices;
    std::ignore = approval::iter_ones(
        core_indices,
        [&](const auto claimed_core_index) -> outcome::result<void> {
          for (uint32_t candidate_index = 0;
               candidate_index < block_entry.candidates.size();
               ++candidate_index) {
            const auto &[core_index, _] =
                block_entry.candidates[candidate_index];
            if (core_index == claimed_core_index) {
              candidate_indices.emplace_back(candidate_index);
              return outcome::success();
            }
          }
          return outcome::success();
        });

    scale::BitVec v;
    for (const auto candidate_index : candidate_indices) {
      if (candidate_index >= v.bits.size()) {
        v.bits.resize(candidate_index + 1);
      }
      v.bits[candidate_index] = true;
    }

    if (v.bits.empty()) {
      return std::nullopt;
    }

    return v;
  }

  void ApprovalDistribution::imported_block_info(
      const primitives::BlockHash &block_hash,
      const primitives::BlockHeader &block_header) {
    REINVOKE(
        *worker_pool_handler_, imported_block_info, block_hash, block_header);

    auto call = [&]() -> outcome::result<NewHeadDataContext> {
      OUTCOME_TRY(included_candidates, request_included_candidates(block_hash));
      OUTCOME_TRY(
          index_and_pair,
          request_session_index_and_info(block_hash, block_header.parent_hash));
      OUTCOME_TRY(
          block_and_header,
          request_babe_epoch_and_block_header(block_header, block_hash));
      return std::make_tuple(std::move(included_candidates),
                             std::move(index_and_pair),
                             std::move(block_and_header));
    };

    if (auto res = call(); res.has_value()) {
      storeNewHeadContext(block_hash, std::move(res.value()));
    } else {
      SL_ERROR(logger_,
               "Error while retrieve neccessary data.(error={})",
               res.error());
    }
  }

  void ApprovalDistribution::storeNewHeadContext(
      const primitives::BlockHash &block_hash, NewHeadDataContext &&context) {
    REINVOKE(*approval_thread_handler_,
             storeNewHeadContext,
             block_hash,
             std::move(context));

    for_ACU(
        block_hash, [this, block_hash, context{std::move(context)}](auto &acu) {
          auto &&[included, session, babe_config] = std::move(context);
          auto &&[session_index, session_info] = std::move(session);
          auto &&[epoch_number, babe_block_header, authorities, randomness] =
              std::move(babe_config);

          acu.second.included_candidates = std::move(included);
          acu.second.babe_epoch = epoch_number;
          acu.second.babe_block_header = std::move(babe_block_header);
          acu.second.authorities = std::move(authorities);
          acu.second.randomness = std::move(randomness);

          this->try_process_approving_context(
              acu, block_hash, session_index, session_info);
        });
  }

  template <typename Func>
  void ApprovalDistribution::for_ACU(const primitives::BlockHash &block_hash,
                                     Func &&func) {
    BOOST_ASSERT(approval_thread_handler_->isInCurrentThread());
    if (auto it = approving_context_map_.find(block_hash);
        it != approving_context_map_.end()) {
      std::forward<Func>(func)(*it);
    }
  }

  void ApprovalDistribution::try_process_approving_context(
      ApprovalDistribution::ApprovingContextUnit &acu,
      const primitives::BlockHash &block_hash,
      SessionIndex session_index,
      const runtime::SessionInfo &session_info) {
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

    bool enable_v2_assignments = false;
    if (auto r = parachain_host_->node_features(block_hash, session_index);
        r.has_value()) {
      if (r.value()
          && r.value()->bits.size() > runtime::ParachainHost::NodeFeatureIndex::
                     EnableAssignmentsV2) {
        enable_v2_assignments =
            r.value()->bits
                [runtime::ParachainHost::NodeFeatureIndex::EnableAssignmentsV2];
      }
    }

    approval::UnsafeVRFOutput unsafe_vrf{
        .vrf_output = ac.babe_block_header->vrf_output,
        .slot = ac.babe_block_header->slot_number,
        .authority_index = ac.babe_block_header->authority_index};

    ::RelayVRFStory relay_vrf;
    if (auto res = unsafe_vrf.compute_randomness(
            relay_vrf, *ac.authorities, *ac.randomness, *ac.babe_epoch);
        res.has_error()) {
      logger_->warn("Relay VRF return error.(error={})", res.error());
      return;
    }

    auto assignments = compute_assignments(keystore_,
                                           session_info,
                                           relay_vrf,
                                           *ac.included_candidates,
                                           enable_v2_assignments,
                                           logger_);

    /// TODO(iceseer): force approve impl

    ac.complete_callback(ImportedBlockInfo{
        .included_candidates = std::move(*ac.included_candidates),
        .session_index = session_index,
        .assignments = std::move(assignments),
        .n_validators = session_info.validators.size(),
        .relay_vrf_story = relay_vrf,
        .slot = unsafe_vrf.slot,
        .force_approve = std::nullopt});
  }

  std::optional<
      std::pair<std::reference_wrapper<
                    kagome::parachain::ApprovalDistribution::ApprovalEntry>,
                kagome::parachain::approval::ApprovalStatus>>
  ApprovalDistribution::approval_status(const BlockEntry &block_entry,
                                        CandidateEntry &candidate_entry) {
    std::optional<runtime::SessionInfo> opt_session_info{};
    if (auto session_info_res = parachain_host_->session_info(
            block_entry.parent_hash, block_entry.session);
        session_info_res.has_value()) {
      opt_session_info = std::move(session_info_res.value());
    } else {
      logger_->warn(
          "Approval status. Session info runtime request failed. "
          "(block_hash={}, session_index={}, error={})",
          block_entry.parent_hash,
          block_entry.session,
          session_info_res.error());
      return std::nullopt;
    }

    if (!opt_session_info) {
      logger_->debug(
          "Can't obtain SessionInfo. (parent_hash={}, session_index={})",
          block_entry.parent_hash,
          block_entry.session);
      return std::nullopt;
    }

    runtime::SessionInfo &session_info = *opt_session_info;
    const auto block_hash = block_entry.block_hash;

    const auto tranche_now =
        ::trancheNow(config_.slot_duration_millis, block_entry.slot);
    const auto block_tick =
        ::slotNumberToTick(config_.slot_duration_millis, block_entry.slot);
    const auto no_show_duration = ::slotNumberToTick(
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

  outcome::result<std::pair<SessionIndex, runtime::SessionInfo>>
  ApprovalDistribution::request_session_index_and_info(
      const primitives::BlockHash &block_hash,
      const primitives::BlockHash &parent_hash) {
    OUTCOME_TRY(session_index,
                parachain_host_->session_index_for_child(parent_hash));
    OUTCOME_TRY(session_info,
                parachain_host_->session_info(block_hash, session_index));

    if (!session_info) {
      SL_ERROR(logger_,
               "No session info for [session_index: {}, block_hash: {}]",
               session_index,
               block_hash);
      return ApprovalDistributionError::NO_SESSION_INFO;
    }

    SL_TRACE(logger_,
             "Found session info. (block hash={}, session index={}, "
             "validators count={}, assignment keys count={}, "
             "availability cores={}, delay tranches ={})",
             block_hash,
             session_index,
             session_info->validators.size(),
             session_info->assignment_keys.size(),
             session_info->n_cores,
             session_info->n_delay_tranches);
    return std::make_pair(session_index, std::move(*session_info));
  }

  outcome::result<std::tuple<consensus::EpochNumber,
                             consensus::babe::BabeBlockHeader,
                             consensus::babe::Authorities,
                             consensus::Randomness>>
  ApprovalDistribution::request_babe_epoch_and_block_header(
      const primitives::BlockHeader &block_header,
      const primitives::BlockHash &block_hash) {
    OUTCOME_TRY(babe_header, consensus::babe::getBabeBlockHeader(block_header));
    OUTCOME_TRY(epoch,
                slots_util_.get()->slotToEpoch(*block_header.parentInfo(),
                                               babe_header.slot_number));
    OUTCOME_TRY(babe_config,
                babe_config_repo_->config(*block_header.parentInfo(), epoch));

    return std::make_tuple(
        epoch, babe_header, babe_config->authorities, babe_config->randomness);
  }

  outcome::result<ApprovalDistribution::CandidateIncludedList>
  ApprovalDistribution::request_included_candidates(
      const primitives::BlockHash &block_hash) {
    OUTCOME_TRY(candidates, parachain_host_->candidate_events(block_hash));
    ApprovalDistribution::CandidateIncludedList included;

    for (auto &candidate : candidates) {
      if (auto obj{boost::get<runtime::CandidateIncluded>(&candidate)}) {
        included.emplace_back(
            std::make_tuple(HashedCandidateReceipt{obj->candidate_receipt},
                            obj->core_index,
                            obj->group_index));
      }
    }
    return included;
  }

  outcome::result<std::vector<
      std::pair<CandidateHash, ApprovalDistribution::CandidateEntry>>>
  ApprovalDistribution::add_block_entry(
      primitives::BlockNumber block_number,
      const primitives::BlockHash &block_hash,
      const primitives::BlockHash &parent_hash,
      scale::BitVec &&approved_bitfield,
      const ImportedBlockInfo &block_info) {
    std::vector<std::pair<CandidateHash, CandidateEntry>> entries;
    std::vector<std::pair<CoreIndex, CandidateHash>> candidates;
    auto blocks = storedBlocks().get_or_create(block_number);
    if (blocks.get().contains(block_hash)) {
      return entries;
    }
    blocks.get().insert(block_hash);

    entries.reserve(block_info.included_candidates.size());
    candidates.reserve(block_info.included_candidates.size());
    for (const auto &[hashed_candidate_receipt, coreIndex, groupIndex] :
         block_info.included_candidates) {
      std::optional<std::reference_wrapper<const OurAssignment>> assignment{};
      if (auto assignment_it = block_info.assignments.find(coreIndex);
          assignment_it != block_info.assignments.end()) {
        assignment = assignment_it->second;
      }

      auto candidate_entry = storedCandidateEntries().get_or_create(
          hashed_candidate_receipt.getHash(),
          hashed_candidate_receipt.get(),
          block_info.session_index,
          block_info.n_validators);
      candidate_entry.get().block_assignments.insert_or_assign(
          block_hash,
          ApprovalEntry(groupIndex, assignment, block_info.n_validators));
      entries.emplace_back(hashed_candidate_receipt.getHash(),
                           candidate_entry.get());
      candidates.emplace_back(coreIndex, hashed_candidate_receipt.getHash());
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
                   .slot = block_info.slot,
                   .relay_vrf_story = block_info.relay_vrf_story,
                   .candidates = std::move(candidates),
                   .approved_bitfield = std::move(approved_bitfield),
                   .distributed_assignments = {},
                   .children = {}});

    return entries;
  }

  outcome::result<ApprovalDistribution::BlockImportedCandidates>
  ApprovalDistribution::processImportedBlock(
      primitives::BlockNumber block_number,
      const primitives::BlockHash &block_hash,
      const primitives::BlockHash &parent_hash,
      primitives::BlockNumber finalized_block_number,
      ApprovalDistribution::ImportedBlockInfo &&imported_block) {
    SL_TRACE(logger_,
             "Star imported block processing. (block number={}, block hash={}, "
             "parent hash={})",
             block_number,
             block_hash,
             parent_hash);

    OUTCOME_TRY(session_info,
                parachain_host_->session_info(block_hash,
                                              imported_block.session_index));

    if (!session_info) {
      SL_TRACE(logger_,
               "No session info. (block number={}, block hash={}, "
               "parent hash={}, session index={})",
               block_number,
               block_hash,
               parent_hash,
               imported_block.session_index);
      return ApprovalDistributionError::NO_SESSION_INFO;
    }

    const auto block_tick =
        slotNumberToTick(config_.slot_duration_millis, imported_block.slot);

    const auto no_show_duration = slotNumberToTick(config_.slot_duration_millis,
                                                   session_info->no_show_slots);

    const auto needed_approvals = session_info->needed_approvals;
    const auto num_candidates = imported_block.included_candidates.size();

    scale::BitVec approved_bitfield;
    size_t num_ones = 0ull;

    if (0 == needed_approvals) {
      SL_TRACE(logger_, "Insta-approving all candidates. {}", block_hash);
      approved_bitfield.bits.insert(
          approved_bitfield.bits.end(), num_candidates, true);
      num_ones = num_candidates;
    } else {
      approved_bitfield.bits.insert(
          approved_bitfield.bits.end(), num_candidates, false);
      for (size_t ix = 0; ix < imported_block.included_candidates.size();
           ++ix) {
        const auto &[_0, _1, backing_group] =
            imported_block.included_candidates[ix];
        const size_t backing_group_size =
            ((backing_group < session_info->validator_groups.size())
                 ? session_info->validator_groups[backing_group].size()
                 : 0ull);
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
                                imported_block));

    std::vector<CandidateHash> candidates;
    for (const auto &[hashed_candidate_receipt, _1, _2] :
         imported_block.included_candidates) {
      candidates.emplace_back(hashed_candidate_receipt.getHash());
    }

    runNewBlocks(
        approval::BlockApprovalMeta{
            .hash = block_hash,
            .number = block_number,
            .parent_hash = parent_hash,
            .candidates = std::move(candidates),
            .slot = imported_block.slot,
            .session = imported_block.session_index,
        },
        finalized_block_number);

    return BlockImportedCandidates{.block_hash = block_hash,
                                   .block_number = block_number,
                                   .block_tick = block_tick,
                                   .no_show_duration = no_show_duration,
                                   .imported_candidates = std::move(entries)};
  }

  void ApprovalDistribution::runNewBlocks(
      approval::BlockApprovalMeta &&meta,
      primitives::BlockNumber finalized_block_number) {
    std::optional<primitives::BlockHash> new_hash;
    if (!storedDistribBlockEntries().get(meta.hash)) {
      const auto candidates_count = meta.candidates.size();
      std::vector<DistribCandidateEntry> candidates;
      candidates.resize(candidates_count);

      new_hash = meta.hash;
      storedDistribBlockEntries().set(meta.hash,
                                      DistribBlockEntry{
                                          .candidates = std::move(candidates),
                                          .knowledge = {},
                                          .known_by = {},
                                          .number = meta.number,
                                          .parent_hash = meta.parent_hash,
                                          .approval_entries = {},
                                      });
      blocks_by_number_[meta.number].insert(meta.hash);
    }

    std::unordered_set<libp2p::peer::PeerId> active_peers;
    pm_->enumeratePeerState(
        [&](const libp2p::peer::PeerId &peer, network::PeerState &_) {
          active_peers.insert(peer);
          return true;
        });

    network::View our_current_view{
        .heads_ = block_tree_->getLeaves(),
        .finalized_number_ = block_tree_->getLastFinalized().number,
    };

    approval_thread_handler_->execute([wself{weak_from_this()},
                                       our_current_view{
                                           std::move(our_current_view)},
                                       active_peers{std::move(active_peers)},
                                       new_hash,
                                       meta{std::move(meta)}]() {
      if (auto self = wself.lock()) {
        SL_TRACE(self->logger_, "Got new block.(hash={})", new_hash);
        // std::unordered_map<libp2p::peer::PeerId, network::View>
        // peer_views(std::move(self->peer_views_));
        for (auto it = self->peer_views_.begin();
             it != self->peer_views_.end();) {
          if (active_peers.contains(it->first)) {
            ++it;
          } else {
            it = self->peer_views_.erase(it);
          }
        }

        for (const auto &p : active_peers) {
          std::ignore = self->peer_views_[p];
        }

        for (const auto &[peed_id, view] : self->peer_views_) {
          network::View view_intersection{
              .heads_ = {},
              .finalized_number_ = view.finalized_number_,
          };

          if (new_hash && view.contains(*new_hash)) {
            view_intersection.heads_.emplace_back(*new_hash);
          }

          self->unify_with_peer(self->storedDistribBlockEntries(),
                                peed_id,
                                view_intersection,
                                false);
        }

        for (auto it = self->pending_known_.begin();
             it != self->pending_known_.end();) {
          if (!self->storedDistribBlockEntries().get(it->first)) {
            ++it;
          } else {
            SL_TRACE(self->logger_,
                     "Processing pending assignment/approvals.(count={})",
                     it->second.size());
            for (auto &item : it->second) {
              visit_in_place(
                  item.second,
                  [&](const network::vstaging::Assignment &assignment) {
                    self->import_and_circulate_assignment(
                        item.first,
                        assignment.indirect_assignment_cert,
                        assignment.candidate_bitfield);
                  },
                  [&](const network::vstaging::IndirectSignedApprovalVoteV2
                          &approval) {
                    self->import_and_circulate_approval(item.first, approval);
                  });
            }
            it = self->pending_known_.erase(it);
          }
        }
      }
    });
  }

  template <typename Func>
  void ApprovalDistribution::handle_new_head(const primitives::BlockHash &head,
                                             const network::ExView &updated,
                                             Func &&func) {
    BOOST_ASSERT(approval_thread_handler_->isInCurrentThread());

    const auto block_number = updated.new_head.number;
    auto parent_hash{updated.new_head.parent_hash};
    if (approving_context_map_.contains(head)) {
      logger_->warn("Approving {} already in progress.", head);
      return;  // Error::ALREADY_APPROVING;
    }

    approving_context_map_.emplace(
        head,
        ApprovingContext{
            .block_header = updated.new_head,
            .included_candidates = std::nullopt,
            .babe_block_header = std::nullopt,
            .babe_epoch = std::nullopt,
            .randomness = {},
            .authorities = {},
            .complete_callback =
                [wself{weak_from_this()},
                 block_hash{head},
                 block_number,
                 finalized_block_number{updated.view.finalized_number_},
                 parent_hash,
                 func = std::forward<Func>(func)](
                    outcome::result<ImportedBlockInfo> &&block_info) mutable {
                  auto self = wself.lock();
                  if (!self) {
                    return;
                  }

                  if (block_info.has_error()) {
                    SL_WARN(self->logger_,
                            "ImportedBlockInfo request failed: {}",
                            block_info.error());
                    return;
                  }

                  std::forward<Func>(func)(self->processImportedBlock(
                      block_number,
                      block_hash,
                      parent_hash,
                      finalized_block_number,
                      std::move(block_info.value())));
                }});

    imported_block_info(head, updated.new_head);
  }

  void ApprovalDistribution::on_active_leaves_update(
      const network::ExView &updated) {
    REINVOKE(*approval_thread_handler_, on_active_leaves_update, updated);

    if (!parachain_processor_->canProcessParachains()) {
      return;
    }

    const auto &relay_parent = updated.new_head.hash();

    if (!storedDistribBlockEntries().get(relay_parent)) {
      [[maybe_unused]] auto &_ = pending_known_[relay_parent];
    }

    handle_new_head(
        relay_parent,
        updated,
        [wself{weak_from_this()},
         head{relay_parent}](auto &&possible_candidate) {
          if (auto self = wself.lock()) {
            if (possible_candidate.has_error()) {
              SL_ERROR(self->logger_,
                       "Internal error while retrieve block imported "
                       "candidates: {}",
                       possible_candidate.error());
              return;
            }

            BOOST_ASSERT(self->approval_thread_handler_->isInCurrentThread());
            self->scheduleTranche(head, std::move(possible_candidate.value()));
          }
        });
  }

  void ApprovalDistribution::launch_approval(
      const RelayHash &relay_block_hash,
      SessionIndex session_index,
      const HashedCandidateReceipt &hashed_candidate,
      ValidatorIndex validator_index,
      Hash block_hash,
      std::optional<CoreIndex> core,
      GroupIndex backing_group) {
    auto on_recover_complete =
        [wself{weak_from_this()},
         hashed_candidate{hashed_candidate},
         block_hash,
         session_index,
         validator_index,
         relay_block_hash](
            std::optional<outcome::result<runtime::AvailableData>>
                &&opt_result) mutable {
          auto self = wself.lock();
          if (!self) {
            return;
          }

          const auto &candidate_receipt = hashed_candidate.get();
          if (!opt_result) {  // Unavailable
            self->logger_->warn(
                "No available parachain data.(session index={}, candidate "
                "hash={}, relay block hash={})",
                session_index,
                hashed_candidate.getHash(),
                relay_block_hash);
            return;
          }

          if (opt_result->has_error()) {
            self->logger_->warn(
                "Parachain data recovery failed.(error={}, session index={}, "
                "candidate hash={}, relay block hash={})",
                opt_result->error(),
                session_index,
                hashed_candidate.getHash(),
                relay_block_hash);
            self->dispute_coordinator_.get()->issueLocalStatement(
                session_index,
                hashed_candidate.getHash(),
                hashed_candidate.get(),
                false);
            return;
          }
          auto &available_data = opt_result->value();
          auto result = self->parachain_host_->validation_code_by_hash(
              block_hash, candidate_receipt.descriptor.validation_code_hash);
          if (result.has_error() || !result.value()) {
            self->logger_->warn(
                "Approval state is failed. Block hash {}, session index {}, "
                "validator index {}, relay parent {}",
                block_hash,
                session_index,
                validator_index,
                candidate_receipt.descriptor.relay_parent);
            return;  /// ApprovalState::failed
          }

          self->logger_->info(
              "Make exhaustive validation. Candidate hash {}, validator index "
              "{}, block hash {}",
              hashed_candidate.getHash(),
              validator_index,
              block_hash);

          runtime::ValidationCode &validation_code = *result.value();

          auto cb = [weak_self{wself},
                     hashed_candidate,
                     session_index,
                     validator_index](outcome::result<Pvf::Result> outcome) {
            auto self = weak_self.lock();
            if (not self) {
              return;
            }
            const auto &candidate_receipt = hashed_candidate.get();

            std::vector<Hash> advence_hashes;
            self->approvals_cache_.exclusiveAccess([&](auto &approvals_cache) {
              if (auto it = approvals_cache.find(hashed_candidate.getHash());
                  it != approvals_cache.end()) {
                ApprovalCache &ac = it->second;
                advence_hashes.assign(ac.blocks_.begin(), ac.blocks_.end());
                ac.approval_result = outcome.has_error()
                                       ? ApprovalOutcome::Failed
                                       : ApprovalOutcome::Approved;
              }
            });
            if (outcome.has_error()) {
              self->logger_->warn(
                  "Approval validation failed.(parachain id={}, relay "
                  "parent={}, error={})",
                  candidate_receipt.descriptor.para_id,
                  candidate_receipt.descriptor.relay_parent,
                  outcome.error());
              self->dispute_coordinator_.get()->issueLocalStatement(
                  session_index,
                  hashed_candidate.getHash(),
                  candidate_receipt,
                  false);
            } else {
              for (const auto &b : advence_hashes) {
                self->issue_approval(
                    hashed_candidate.getHash(), validator_index, b);
              }
            }
          };
          self->pvf_->pvfValidate(available_data.validation_data,
                                  available_data.pov,
                                  candidate_receipt,
                                  validation_code,
                                  runtime::PvfExecTimeoutKind::Approval,
                                  std::move(cb));
        };

    recovery_->recover(hashed_candidate,
                       session_index,
                       backing_group,
                       core,
                       std::move(on_recover_complete));
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
      const approval::IndirectAssignmentCertV2 &assignment,
      const scale::BitVec &candidate_indices) {
    BOOST_ASSERT(approval_thread_handler_->isInCurrentThread());
    const auto tick_now = ::tickNow();

    GET_OPT_VALUE_OR_EXIT(block_entry,
                          AssignmentCheckResult::Bad,
                          storedBlockEntries().get(assignment.block_hash));

    std::optional<runtime::SessionInfo> opt_session_info{};
    if (auto session_info_res = parachain_host_->session_info(
            block_entry.parent_hash, block_entry.session);
        session_info_res.has_value()) {
      opt_session_info = std::move(session_info_res.value());
    } else {
      SL_WARN(logger_,
              "Assignment. Session info runtime request failed. "
              "(parent_hash={}, session_index={}, error={})",
              block_entry.parent_hash,
              block_entry.session,
              session_info_res.error());
      return AssignmentCheckResult::Bad;
    }

    if (!opt_session_info) {
      logger_->debug(
          "Can't obtain SessionInfo. (parent_hash={}, session_index={})",
          block_entry.parent_hash,
          block_entry.session);
      return AssignmentCheckResult::Bad;
    }

    runtime::SessionInfo &session_info = *opt_session_info;
    const auto n_cores = size_t(session_info.n_cores);

    // Early check the candidate bitfield and core bitfields lengths <
    // `n_cores`. Core bitfield length is checked later in
    // `check_assignment_cert`.
    if (candidate_indices.bits.size() > n_cores) {
      SL_TRACE(logger_,
               "Oversized bitfield. (validator={}, n_cores={}, "
               "candidate_bitfield_len={})",
               assignment.validator,
               n_cores,
               candidate_indices.bits.size());
      return AssignmentCheckResult::Bad;
    }

    std::vector<GroupIndex> backing_groups;
    std::vector<CoreIndex> claimed_core_indices;
    std::vector<CandidateHash> assigned_candidate_hashes;

    for (size_t candidate_index = 0;
         candidate_index < candidate_indices.bits.size();
         ++candidate_index) {
      if (!candidate_indices.bits[candidate_index]) {
        continue;
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

      backing_groups.emplace_back(approval_entry.backing_group);
      claimed_core_indices.emplace_back(claimed_core_index);
      assigned_candidate_hashes.emplace_back(assigned_candidate_hash);
    }

    // Error on null assignments.
    if (claimed_core_indices.empty()) {
      return AssignmentCheckResult::Bad;
    }

    scale::BitVec v;
    for (const auto ci : claimed_core_indices) {
      if (ci >= v.bits.size()) {
        v.bits.resize(ci + 1);
      }
      v.bits[ci] = true;
    }

    DelayTranche tranche;  // NOLINT(cppcoreguidelines-init-variables)
    if (auto res = checkAssignmentCert(v,
                                       assignment.validator,
                                       session_info,
                                       block_entry.relay_vrf_story,
                                       assignment.cert,
                                       backing_groups);
        res.has_value()) {
      const auto current_tranche =
          ::trancheNow(config_.slot_duration_millis, block_entry.slot);
      const auto too_far_in_future =
          current_tranche + DelayTranche(kTickTooFarInFuture);
      if (res.value() >= too_far_in_future) {
        return AssignmentCheckResult::TooFarInFuture;
      }
      tranche = res.value();
    } else {
      logger_->warn("Check assignment certificate failed.(error={})",
                    res.error());
      return AssignmentCheckResult::Bad;
    }

    bool is_duplicate = true;
    for (size_t candidate_index = 0, h_i = 0;
         candidate_index < candidate_indices.bits.size();
         ++candidate_index) {
      if (!candidate_indices.bits[candidate_index]) {
        continue;
      }
      const auto assigned_candidate_hash = assigned_candidate_hashes[h_i++];
      GET_OPT_VALUE_OR_EXIT(
          candidate_entry,
          AssignmentCheckResult::Bad,
          storedCandidateEntries().get(assigned_candidate_hash));

      GET_OPT_VALUE_OR_EXIT(
          approval_entry,
          AssignmentCheckResult::Bad,
          candidate_entry.approval_entry(assignment.block_hash));

      is_duplicate =
          is_duplicate && approval_entry.is_assigned(assignment.validator);
      approval_entry.import_assignment(tranche, assignment.validator, tick_now);

      if (auto result = approval_status(block_entry, candidate_entry); result) {
        schedule_wakeup_action(result->first.get(),
                               block_entry.block_hash,
                               block_entry.block_number,
                               assigned_candidate_hash,
                               result->second.block_tick,
                               tick_now,
                               result->second.required_tranches);
      }
    }

    if (is_duplicate) {
      return AssignmentCheckResult::AcceptedDuplicate;
    }
    if (approval::count_ones(candidate_indices) > 1) {
      SL_TRACE(logger_,
               "Imported assignment for multiple cores. (validator={})",
               assignment.validator);
      return AssignmentCheckResult::Accepted;
    }
    SL_TRACE(logger_,
             "Imported assignment for a single core. (validator={})",
             assignment.validator);
    return AssignmentCheckResult::Accepted;
  }

  ApprovalDistribution::ApprovalCheckResult
  ApprovalDistribution::check_and_import_approval(
      const approval::IndirectSignedApprovalVoteV2 &approval) {
    GET_OPT_VALUE_OR_EXIT(
        block_entry,
        ApprovalCheckResult::Bad,
        storedBlockEntries().get(approval.payload.payload.block_hash));

    std::vector<std::pair<size_t, CandidateHash>> approved_candidates_info;
    auto r = approval::iter_ones(
        approval.payload.payload.candidate_indices,
        [&](const auto candidate_index) -> outcome::result<void> {
          if (candidate_index >= block_entry.candidates.size()) {
            SL_WARN(logger_,
                    "Candidate index more than candidates array.(candidate "
                    "index={})",
                    candidate_index);
            return ApprovalDistributionError::CANDIDATE_INDEX_OUT_OF_BOUNDS;
          }
          const auto &candidate = block_entry.candidates[candidate_index];
          approved_candidates_info.emplace_back(candidate_index,
                                                candidate.second);
          return outcome::success();
        });

    if (r.has_error()) {
      return ApprovalCheckResult::Bad;
    }

    std::optional<runtime::SessionInfo> opt_session_info;
    if (auto session_info_res = parachain_host_->session_info(
            approval.payload.payload.block_hash, block_entry.session);
        session_info_res.has_value()) {
      opt_session_info = std::move(session_info_res.value());
    } else {
      logger_->warn(
          "Approval. Session info runtime request failed. (block_hash={}, "
          "session_index={}, error={})",
          approval.payload.payload.block_hash,
          block_entry.session,
          session_info_res.error());
      return ApprovalCheckResult::Bad;
    }

    if (!opt_session_info) {
      logger_->debug(
          "Can't obtain SessionInfo. (parent_hash={}, session_index={})",
          approval.payload.payload.block_hash,
          block_entry.session);
      return ApprovalCheckResult::Bad;
    }

    runtime::SessionInfo &session_info = *opt_session_info;
    const auto &pubkey = session_info.validators[approval.payload.ix];

    for (const auto &[approval_candidate_index, approved_candidate_hash] :
         approved_candidates_info) {
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

      SL_DEBUG(logger_,
               "Importing approval vote.(validator index={}, validator id={}, "
               "candidate hash={}, para id={})",
               approval.payload.ix,
               pubkey,
               approved_candidate_hash,
               candidate_entry.candidate.get().descriptor.para_id);
      advance_approval_state(block_entry,
                             approved_candidate_hash,
                             candidate_entry,
                             approval::RemoteApproval{
                                 .validator_ix = approval.payload.ix,
                             });
    }
    return ApprovalCheckResult::Accepted;
  }

#undef GET_OPT_VALUE_OR_EXIT

  void ApprovalDistribution::import_and_circulate_assignment(
      const MessageSource &source,
      const approval::IndirectAssignmentCertV2 &assignment,
      const scale::BitVec &claimed_candidate_indices) {
    BOOST_ASSERT(approval_thread_handler_->isInCurrentThread());

    const auto &block_hash = assignment.block_hash;
    const auto validator_index = assignment.validator;

    auto opt_entry = storedDistribBlockEntries().get(block_hash);
    if (!opt_entry) {
      logger_->warn(
          "Unexpected assignment. (peer id={}, block hash={}, validator "
          "index={})",
          source ? fmt::format("{}", source->get()) : "our",
          block_hash,
          validator_index);
      return;
    }
    auto &entry = opt_entry->get();

    SL_DEBUG(
        logger_,
        "Import assignment. (peer id={}, block hash={}, validator index={})",
        source ? fmt::format("{}", source->get()) : "our",
        block_hash,
        validator_index);

    // Compute metadata on the assignment.
    auto message_subject{std::make_tuple(
        block_hash, claimed_candidate_indices, validator_index)};
    auto message_kind{approval::MessageKind::Assignment};

    if (source) {
      const auto &peer_id = source->get();

      if (auto it = entry.known_by.find(peer_id); it != entry.known_by.end()) {
        if (auto &peer_knowledge = it->second;
            peer_knowledge.contains(message_subject, message_kind)) {
          if (!peer_knowledge.received.insert(message_subject, message_kind)) {
            SL_TRACE(logger_,
                     "Duplicate assignment. (peer id={}, block_hash={}, "
                     "validator index={})",
                     peer_id,
                     std::get<0>(message_subject),
                     std::get<2>(message_subject));
          }
          return;
        }
      } else {
        SL_TRACE(logger_,
                 "Assignment from a peer is out of view. (peer id={}, "
                 "block_hash={}, validator index={})",
                 peer_id,
                 std::get<0>(message_subject),
                 std::get<2>(message_subject));
      }

      /// if the assignment is known to be valid, reward the peer
      if (entry.knowledge.contains(message_subject, message_kind)) {
        /// TODO(iceseer): modify reputation
        if (auto it = entry.known_by.find(peer_id);
            it != entry.known_by.end()) {
          SL_TRACE(logger_, "Known assignment. (peer id={})", peer_id);
          it->second.received.insert(message_subject, message_kind);
        }
      }

      switch (
          check_and_import_assignment(assignment, claimed_candidate_indices)) {
        case AssignmentCheckResult::Accepted: {
          SL_TRACE(logger_,
                   "Assignment accepted. (peer id={}, block hash={})",
                   source->get(),
                   block_hash);
          entry.knowledge.known_messages[message_subject] = message_kind;
          if (auto it = entry.known_by.find(peer_id);
              it != entry.known_by.end()) {
            it->second.received.insert(message_subject, message_kind);
          }
        } break;
        case AssignmentCheckResult::Bad: {
          SL_WARN(logger_,
                  "Got bad assignment from peer. (peer id={}, block hash={})",
                  source->get(),
                  block_hash);
        }
          return;
        case AssignmentCheckResult::TooFarInFuture: {
          SL_TRACE(
              logger_,
              "Got an assignment too far in the future. (peer id={}, block "
              "hash={})",
              source->get(),
              block_hash);
        }
          return;
        case AssignmentCheckResult::AcceptedDuplicate: {
          if (auto it = entry.known_by.find(peer_id);
              it != entry.known_by.end()) {
            auto &peer_knowledge = it->second;
            peer_knowledge.received.insert(message_subject, message_kind);
          }
          SL_TRACE(logger_,
                   "Got an `AcceptedDuplicate` assignment. (peer id={}, block "
                   "hash={})",
                   source->get(),
                   block_hash);
        }
          return;
      }
    } else {
      if (!entry.knowledge.insert(message_subject, message_kind)) {
        SL_WARN(logger_,
                "Importing locally an already known assignment. "
                "(block_hash={}, validator index={})",
                std::get<0>(message_subject),
                std::get<2>(message_subject));
        return;
      }
      SL_TRACE(logger_,
               "Importing locally a new assignment. (block_hash={}, validator "
               "index={})",
               std::get<0>(message_subject),
               std::get<2>(message_subject));
    }

    const auto local = !source.has_value();
    auto &approval_entry = entry.insert_approval_entry(DistribApprovalEntry{
        .assignment = assignment,
        .assignment_claimed_candidates = claimed_candidate_indices,
        .approvals = {},
        .validator_index = assignment.validator,
        .routing_info =
            ApprovalRouting{
                .required_routing =
                    grid::RequiredRouting{
                        .value =
                            grid::RequiredRouting::All},  /// TODO(iceseer):
                                                          /// calculate
                                                          /// based on grid
                .local = local,
                .random_routing = {},
                .peers_randomly_routed = {},
            },
    });

    const auto n_peers_total = peer_view_->peersCount();
    auto peer_filter = [&](const auto &peer, const auto &peer_kn) {
      if (!source || peer != source->get()) {
        const auto route_random =
            approval_entry.routing_info.random_routing.sample(n_peers_total);
        if (route_random) {
          approval_entry.routing_info.mark_randomly_sent(peer);
          return true;
        }
      }
      return false;
    };

    std::unordered_set<libp2p::peer::PeerId> peers{};
    for (auto &[peer_id, peer_knowledge] : entry.known_by) {
      if (peer_filter(peer_id, peer_knowledge)) {
        peers.insert(peer_id);
        peer_knowledge.sent.insert(message_subject, message_kind);
      }
    }

    if (!peers.empty()) {
      runDistributeAssignment(
          assignment, claimed_candidate_indices, std::move(peers));
    }
  }

  void ApprovalDistribution::import_and_circulate_approval(
      const MessageSource &source,
      const approval::IndirectSignedApprovalVoteV2 &vote) {
    BOOST_ASSERT(approval_thread_handler_->isInCurrentThread());
    const auto &block_hash = vote.payload.payload.block_hash;
    const auto validator_index = vote.payload.ix;
    const auto &candidate_indices = vote.payload.payload.candidate_indices;

    auto opt_entry = storedDistribBlockEntries().get(block_hash);
    if (!opt_entry) {
      logger_->info(
          "Unexpected approval. (peer id={}, block hash={}, validator "
          "index={})",
          source ? fmt::format("{}", source->get()) : "our",
          block_hash,
          validator_index);
      return;
    }

    SL_DEBUG(logger_,
             "Import approval. (peer id={}, block hash={}, validator index={})",
             source ? fmt::format("{}", source->get()) : "our",
             block_hash,
             validator_index);

    auto &entry = opt_entry->get();
    auto message_subject{
        std::make_tuple(block_hash, candidate_indices, validator_index)};
    auto message_kind{approval::MessageKind::Approval};

    if (source) {
      const auto &peer_id = source->get();
      if (!entry.knowledge.contains(message_subject,
                                    approval::MessageKind::Assignment)) {
        SL_TRACE(logger_,
                 "Unknown approval assignment. (peer id={}, block hash={}, "
                 "validator={})",
                 peer_id,
                 std::get<0>(message_subject),
                 std::get<2>(message_subject));
        return;
      }

      // check if our knowledge of the peer already contains this approval
      if (auto it = entry.known_by.find(peer_id); it != entry.known_by.end()) {
        if (auto &peer_knowledge = it->second;
            peer_knowledge.contains(message_subject, message_kind)) {
          if (!peer_knowledge.received.insert(message_subject, message_kind)) {
            SL_TRACE(logger_,
                     "Duplicate approval. (peer id={}, block_hash={}, "
                     "validator index={})",
                     peer_id,
                     std::get<0>(message_subject),
                     std::get<2>(message_subject));
          }
          return;
        }
      } else {
        SL_TRACE(logger_,
                 "Approval from a peer is out of view. (peer id={}, "
                 "block_hash={}, validator index={})",
                 peer_id,
                 std::get<0>(message_subject),
                 std::get<2>(message_subject));
      }

      /// if the approval is known to be valid, reward the peer
      if (entry.knowledge.contains(message_subject, message_kind)) {
        SL_TRACE(logger_,
                 "Known approval. (peer id={}, block hash={}, validator={})",
                 peer_id,
                 std::get<0>(message_subject),
                 std::get<2>(message_subject));

        if (auto it = entry.known_by.find(peer_id);
            it != entry.known_by.end()) {
          it->second.received.insert(message_subject, message_kind);
        }
        return;
      }

      switch (check_and_import_approval(vote)) {
        case ApprovalCheckResult::Accepted: {
          entry.knowledge.insert(message_subject, message_kind);
          if (auto it = entry.known_by.find(peer_id);
              it != entry.known_by.end()) {
            it->second.received.insert(message_subject, message_kind);
          }
        } break;
        case ApprovalCheckResult::Bad: {
          logger_->warn(
              "Got a bad approval from peer. (peer id={}, block hash={})",
              source->get(),
              block_hash);
        }
          return;
      }
    } else {
      if (!entry.knowledge.insert(message_subject, message_kind)) {
        // if we already imported an approval, there is no need to distribute it
        // again
        SL_WARN(logger_,
                "Importing locally an already known approval. "
                "(block_hash={}, validator index={})",
                std::get<0>(message_subject),
                std::get<2>(message_subject));
        return;
      }
      SL_TRACE(logger_,
               "Importing locally a new approval. (block_hash={}, validator "
               "index={})",
               std::get<0>(message_subject),
               std::get<2>(message_subject));
    }

    auto res = entry.note_approval(vote);
    if (res.has_error()) {
      SL_WARN(logger_,
              "Possible bug: Vote import failed. (hash={}, validator_index={}, "
              "error={})",
              block_hash,
              validator_index,
              res.error());
      return;
    }
    auto nar = std::move(res.value());

    auto peer_filter = [&](const auto &peer, const auto &peer_kn) {
      const auto &[_, pr] = nar;
      if (source && peer == source->get()) {
        return false;
      }

      /// TODO(iceseer): topology
      return pr.contains(peer);
    };

    std::unordered_set<libp2p::peer::PeerId> peers{};
    for (auto &[peer_id, peer_knowledge] : entry.known_by) {
      if (peer_filter(peer_id, peer_knowledge)) {
        peers.insert(peer_id);
        peer_knowledge.sent.insert(message_subject, message_kind);
      }
    }

    if (!peers.empty()) {
      runDistributeApproval(vote, std::move(peers));
    }
  }

  network::vstaging::Approvals ApprovalDistribution::sanitize_v1_approvals(
      const network::Approvals &approvals) {
    network::vstaging::Approvals sanitized_approvals;
    for (const auto &approval : approvals.approvals) {
      if (size_t(approval.payload.payload.candidate_index) > kMaxBitfieldSize) {
        SL_DEBUG(logger_,
                 "Bad approval v1, invalid candidate index. (block_hash={}, "
                 "candidate_index={})",
                 approval.payload.payload.block_hash,
                 approval.payload.payload.candidate_index);
      } else {
        sanitized_approvals.approvals.emplace_back(
            ::kagome::parachain::approval::from(approval));
      }
    }
    return sanitized_approvals;
  }

  network::vstaging::Assignments ApprovalDistribution::sanitize_v1_assignments(
      const network::Assignments &assignments) {
    network::vstaging::Assignments sanitized_assignments;
    for (const auto &assignment : assignments.assignments) {
      const auto &cert = assignment.indirect_assignment_cert;
      const auto &candidate_index = assignment.candidate_ix;

      const auto cert_bitfield_bits = visit_in_place(
          cert.cert.kind,
          [](const approval::RelayVRFDelay &v) {
            return size_t(v.core_index) + 1;
          },
          [&](const approval::RelayVRFModulo &v) {
            return size_t(candidate_index) + 1;
          });

      const auto candidate_bitfield_bits = size_t(candidate_index) + 1;
      if (cert_bitfield_bits > kMaxBitfieldSize
          || candidate_bitfield_bits > kMaxBitfieldSize) {
        SL_DEBUG(logger_,
                 "Bad assignment v1, invalid candidate index. (block_hash={}, "
                 "candidate_index={}, validator_index={})",
                 cert.block_hash,
                 candidate_index,
                 cert.validator);
      } else {
        scale::BitVec v;
        v.bits.resize(candidate_index + 1);
        v.bits[candidate_index] = true;
        sanitized_assignments.assignments.emplace_back(
            network::vstaging::Assignment{
                .indirect_assignment_cert =
                    approval::IndirectAssignmentCertV2::from(cert),
                .candidate_bitfield = std::move(v),
            });
      }
    }
    return sanitized_assignments;
  }

  void ApprovalDistribution::getApprovalSignaturesForCandidate(
      const CandidateHash &candidate_hash,
      SignaturesForCandidateCallback &&callback) {
    REINVOKE(*approval_thread_handler_,
             getApprovalSignaturesForCandidate,
             candidate_hash,
             std::move(callback));

    if (!parachain_processor_->canProcessParachains()) {
      callback(SignaturesForCandidate{});
      return;
    }

    auto r = storedCandidateEntries().get(candidate_hash);
    if (!r) {
      SL_DEBUG(logger_,
               "Sent back empty votes because the candidate was not found in "
               "db. (candidate={})",
               candidate_hash);
      callback(SignaturesForCandidate{});
      return;
    }
    auto &entry = r->get();

    SignaturesForCandidate all_sigs;
    for (const auto &[hash, _] : entry.block_assignments) {
      auto block_entry = storedBlockEntries().get(hash);
      if (!block_entry) {
        SL_DEBUG(logger_,
                 "Block entry for assignment missing. (candidate={}, hash={})",
                 candidate_hash,
                 hash);
        continue;
      }

      for (size_t candidate_index = 0ull;
           candidate_index < block_entry->get().candidates.size();
           ++candidate_index) {
        const auto &[_core_index, c_hash] =
            block_entry->get().candidates[candidate_index];
        if (c_hash == candidate_hash) {
          const auto index = candidate_index;
          auto distrib_block_entry = storedDistribBlockEntries().get(hash);
          if (!distrib_block_entry) {
            SL_DEBUG(logger_,
                     "`getApprovalSignaturesForCandidate`: could not find "
                     "block entry for given hash!. (hash={})",
                     hash);
            continue;
          }

          for (auto approval :
               distrib_block_entry->get().approval_votes(index)) {
            std::vector<CandidateIndex> ixs;
            std::ignore = approval::iter_ones(
                getPayload(approval).candidate_indices,
                [&](const auto val) -> outcome::result<void> {
                  ixs.emplace_back(val);
                  return outcome::success();
                });

            all_sigs[approval.payload.ix] =
                std::make_tuple(hash, std::move(ixs), approval.signature);
          }
        }
      }
    }
    callback(std::move(all_sigs));
  }

  void ApprovalDistribution::onValidationProtocolMsg(
      const libp2p::peer::PeerId &peer_id,
      const network::VersionedValidatorProtocolMessage &message) {
    REINVOKE(
        *approval_thread_handler_, onValidationProtocolMsg, peer_id, message);

    if (!parachain_processor_->canProcessParachains()) {
      return;
    }

    auto m = [&]() -> std::optional<std::reference_wrapper<
                       const network::vstaging::ApprovalDistributionMessage>> {
      auto m =
          if_type<const network::vstaging::ValidatorProtocolMessage>(message);
      if (!m) {
        SL_TRACE(logger_, "Received V1 message.(peer_id={})", peer_id);
        return std::nullopt;
      }

      auto a = if_type<const network::vstaging::ApprovalDistributionMessage>(
          m->get());
      if (!a) {
        return std::nullopt;
      }

      return a;
    }();

    if (!m) {
      return;
    }

    visit_in_place(
        m->get(),
        [&](const network::vstaging::Assignments &assignments) {
          SL_TRACE(logger_,
                   "Received assignments.(peer_id={}, count={})",
                   peer_id,
                   assignments.assignments.size());
          for (const auto &assignment : assignments.assignments) {
            if (auto it = pending_known_.find(
                    assignment.indirect_assignment_cert.block_hash);
                it != pending_known_.end()) {
              SL_TRACE(
                  logger_,
                  "Pending assignment.(block hash={}, validator={}, peer={})",
                  assignment.indirect_assignment_cert.block_hash,
                  assignment.indirect_assignment_cert.validator,
                  peer_id);
              it->second.emplace_back(peer_id, PendingMessage{assignment});
              continue;
            }

            import_and_circulate_assignment(peer_id,
                                            assignment.indirect_assignment_cert,
                                            assignment.candidate_bitfield);
          }
        },
        [&](const network::vstaging::Approvals &approvals) {
          SL_TRACE(logger_,
                   "Received approvals.(peer_id={}, count={})",
                   peer_id,
                   approvals.approvals.size());
          for (const auto &approval_vote : approvals.approvals) {
            if (auto it = pending_known_.find(
                    approval_vote.payload.payload.block_hash);
                it != pending_known_.end()) {
              SL_TRACE(
                  logger_,
                  "Pending approval.(block hash={}, validator={}, peer={})",
                  approval_vote.payload.payload.block_hash,
                  approval_vote.payload.ix,
                  peer_id);
              it->second.emplace_back(peer_id, PendingMessage{approval_vote});
              continue;
            }

            import_and_circulate_approval(peer_id, approval_vote);
          }
        },
        [&](const auto &) { UNREACHABLE; });
  }

  void ApprovalDistribution::runDistributeAssignment(
      const approval::IndirectAssignmentCertV2 &indirect_cert,
      const scale::BitVec &candidate_indices,
      std::unordered_set<libp2p::peer::PeerId> &&peers) {
    REINVOKE(*main_pool_handler_,
             runDistributeAssignment,
             indirect_cert,
             candidate_indices,
             std::move(peers));

    SL_DEBUG(logger_,
             "Distributing assignment on candidate (block hash={})",
             indirect_cert.block_hash);

    router_->getValidationProtocol()->write(
        peers,
        network::vstaging::Assignments{
            .assignments = {network::vstaging::Assignment{
                .indirect_assignment_cert = indirect_cert,
                .candidate_bitfield = candidate_indices,
            }},
        });
  }

  void ApprovalDistribution::send_assignments_batched(
      std::deque<network::vstaging::Assignment> &&assignments,
      const libp2p::peer::PeerId &peer_id) {
    REINVOKE(*main_pool_handler_,
             send_assignments_batched,
             std::move(assignments),
             peer_id);

    /** TODO(iceseer): optimize
        std::shared_ptr<network::WireMessage<network::ValidatorProtocolMessage>>
     pack = std::make_shared<
                network::WireMessage<network::ValidatorProtocolMessage>>(
                network::ApprovalDistributionMessage{network::Assignments{
                    .assignments = {},}});
        auto &vp = if_type<network::ValidatorProtocolMessage>(*pack);
        auto &adm = if_type<network::ApprovalDistributionMessage>(vp->get());
        auto &a = if_type<network::Assignments>(adm->get());
        a->get().assignments = std::move(msg_pack);
        se->send(peer, router_->getValidationProtocol(), pack);
     *
    */

    while (!assignments.empty()) {
      auto begin = assignments.begin();
      auto end = (assignments.size() > kMaxAssignmentBatchSize)
                   ? assignments.begin() + kMaxAssignmentBatchSize
                   : assignments.end();
      router_->getValidationProtocol()->write(
          peer_id,
          network::vstaging::Assignments{
              .assignments = std::vector(begin, end),
          });
      assignments.erase(begin, end);
    }
  }

  void ApprovalDistribution::send_approvals_batched(
      std::deque<approval::IndirectSignedApprovalVoteV2> &&approvals,
      const libp2p::peer::PeerId &peer_id) {
    REINVOKE(*main_pool_handler_,
             send_approvals_batched,
             std::move(approvals),
             peer_id);

    /** TODO(iceseer): optimize
        std::shared_ptr<network::WireMessage<network::ValidatorProtocolMessage>>
     pack = std::make_shared<
                network::WireMessage<network::ValidatorProtocolMessage>>(
                network::ApprovalDistributionMessage{network::Approvals{
                    .approvals = {},
                }});
        auto &vp = if_type<network::ValidatorProtocolMessage>(*pack);
        auto &adm = if_type<network::ApprovalDistributionMessage>(vp->get());
        auto &a = if_type<network::Approvals>(adm->get());

        loop {
        a->get().approvals = std::move(msg_pack);
        se->send(peer, router_->getValidationProtocol(), pack);
        }
     *
     *
    */

    while (!approvals.empty()) {
      auto begin = approvals.begin();
      auto end = (approvals.size() > kMaxApprovalBatchSize)
                   ? approvals.begin() + kMaxApprovalBatchSize
                   : approvals.end();
      router_->getValidationProtocol()->write(
          peer_id,
          network::vstaging::Approvals{
              .approvals = std::vector(begin, end),
          });
      approvals.erase(begin, end);
    }
  }

  void ApprovalDistribution::runDistributeApproval(
      const approval::IndirectSignedApprovalVoteV2 &vote,
      std::unordered_set<libp2p::peer::PeerId> &&peers) {
    REINVOKE(
        *main_pool_handler_, runDistributeApproval, vote, std::move(peers));

    SL_INFO(logger_,
            "Sending an approval to peers. (block={}, num peers={})",
            vote.payload.payload.block_hash,
            peers.size());

    router_->getValidationProtocol()->write(peers,
                                            network::vstaging::Approvals{
                                                .approvals = {vote},
                                            });
  }

  void ApprovalDistribution::issue_approval(const CandidateHash &candidate_hash,
                                            ValidatorIndex validator_index,
                                            const RelayHash &block_hash) {
    REINVOKE(*approval_thread_handler_,
             issue_approval,
             candidate_hash,
             validator_index,
             block_hash);

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

    std::optional<runtime::SessionInfo> opt_session_info{};
    if (auto session_info_res = parachain_host_->session_info(
            block_entry.parent_hash, block_entry.session);
        session_info_res.has_value()) {
      opt_session_info = std::move(session_info_res.value());
    } else {
      logger_->warn(
          "Issue approval. Session info runtime request failed. "
          "(block_hash={}, session_index={}, error={})",
          block_entry.parent_hash,
          block_entry.session,
          session_info_res.error());
      return;
    }

    if (!opt_session_info) {
      logger_->debug(
          "Can't obtain SessionInfo. (parent_hash={}, session_index={})",
          block_entry.parent_hash,
          block_entry.session);
      return;
    }

    runtime::SessionInfo &session_info = *opt_session_info;
    if (*candidate_index >= block_entry.candidates.size()) {
      logger_->warn(
          "Received malformed request to approve out-of-bounds candidate index "
          "{} included at block {}",
          *candidate_index,
          block_hash);
      return;
    }

    const auto &c_hash = block_entry.candidates[*candidate_index].second;
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
    const auto session = block_entry.session;
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

    scale::BitVec v;
    v.bits.resize(*candidate_index + 1);
    v.bits[*candidate_index] = true;

    import_and_circulate_approval(
        std::nullopt,
        approval::IndirectSignedApprovalVoteV2{
            .payload =
                {
                    .payload =
                        approval::IndirectApprovalVoteV2{
                            .block_hash = block_hash,
                            .candidate_indices = std::move(v),
                        },
                    .ix = validator_index,
                },
            .signature = *sig,
        });

    /// TODO(iceseer): store state for the dispute
  }

  std::optional<ValidatorSignature> ApprovalDistribution::sign_approval(
      const crypto::Sr25519PublicKey &pubkey,
      SessionIndex session_index,
      const CandidateHash &candidate_hash) {
    auto key_pair =
        keystore_->sr25519().findKeypair(crypto::KeyTypes::PARACHAIN, pubkey);
    if (!key_pair) {
      logger_->warn("No key pair in store for {}", pubkey);
      return std::nullopt;
    }
    static std::array<uint8_t, 4ull> kMagic{'A', 'P', 'P', 'R'};
    auto d = std::make_tuple(kMagic, candidate_hash, session_index);
    auto payload = scale::encode(d).value();

    if (auto res = crypto_provider_->sign(key_pair.value(), payload);
        res.has_value()) {
      return res.value();
    }
    logger_->warn("Unable to sign with {}", pubkey);
    return std::nullopt;
  }

  void ApprovalDistribution::runLaunchApproval(
      const approval::IndirectAssignmentCertV2 &indirect_cert,
      DelayTranche assignment_tranche,
      const RelayHash &relay_block_hash,
      const scale::BitVec &claimed_candidate_indices,
      SessionIndex session,
      const HashedCandidateReceipt &hashed_candidate,
      GroupIndex backing_group,
      std::optional<CoreIndex> core,
      bool distribute_assignment) {
    if (!parachain_processor_->canProcessParachains()) {
      return;
    }

    const auto &block_hash = indirect_cert.block_hash;
    const auto validator_index = indirect_cert.validator;

    if (distribute_assignment) {
      import_and_circulate_assignment(
          std::nullopt, indirect_cert, claimed_candidate_indices);
    }

    std::optional<ApprovalOutcome> approval_state =
        approvals_cache_.exclusiveAccess(
            [&](auto &approvals_cache) -> std::optional<ApprovalOutcome> {
              if (auto it = approvals_cache.find(hashed_candidate.getHash());
                  it != approvals_cache.end()) {
                it->second.blocks_.insert(relay_block_hash);
                return it->second.approval_result;
              }
              approvals_cache.emplace(
                  hashed_candidate.getHash(),
                  ApprovalCache{
                      .blocks_ = {relay_block_hash},
                      .approval_result = ApprovalOutcome::Failed,
                  });
              return std::nullopt;
            });

    if (!approval_state) {
      launch_approval(relay_block_hash,
                      session,
                      hashed_candidate,
                      validator_index,
                      block_hash,
                      core,
                      backing_group);
    } else if (*approval_state == ApprovalOutcome::Approved) {
      issue_approval(hashed_candidate.getHash(), validator_index, block_hash);
    }
  }

  void ApprovalDistribution::schedule_wakeup_action(
      const ApprovalEntry &approval_entry,
      const Hash &block_hash,
      BlockNumber block_number,
      const CandidateHash &candidate_hash,
      Tick block_tick,
      Tick tick_now,
      const approval::RequiredTranches &required_tranches) {
    std::optional<Tick> tick{};
    if (!approval_entry.approved) {
      tick = visit_in_place(
          required_tranches,
          [](const approval::AllRequiredTranche &) -> std::optional<Tick> {
            return std::nullopt;
          },
          [&tick_now](const approval::ExactRequiredTranche &e) {
            auto filter = [](const Tick &t, const Tick &ref) {
              return ((t > ref) ? std::optional<Tick>{t}
                                : std::optional<Tick>{});
            };
            return approval::min_or_some(
                e.next_no_show,
                (e.last_assignment_tick ? filter(
                     *e.last_assignment_tick + kApprovalDelay, tick_now)
                                        : std::optional<Tick>{}));
          },
          [&](const approval::PendingRequiredTranche &e) {
            std::optional<DelayTranche> next_announced{};
            for (const auto &t : approval_entry.tranches) {
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

  void ApprovalDistribution::notifyApproved(const Hash &block_hash) {
    /// Action::NoteApprovedInChainSelection => ChainSelectionMessage::Approved
    if (auto result = block_tree_->markAsParachainDataBlock(block_hash);
        result.has_error()) {
      logger_->warn(
          "Adjust weight for block with parachain data failed.(block hash={}, "
          "error={})",
          block_hash,
          result.error());
    }
  }

  void ApprovalDistribution::advance_approval_state(
      BlockEntry &block_entry,
      const CandidateHash &candidate_hash,
      CandidateEntry &candidate_entry,
      approval::ApprovalStateTransition transition) {
    const auto validator_index = approval::validator_index(transition);
    const std::optional<bool> already_approved_by =
        validator_index ? candidate_entry.mark_approval(*validator_index)
                        : std::optional<bool>{};
    const auto candidate_approved_in_block =
        block_entry.is_candidate_approved(candidate_hash);

    if (!approval::is_local_approval(transition)) {
      if (candidate_approved_in_block) {
        return;
      }
    }

    const auto &block_hash = block_entry.block_hash;
    const auto block_number = block_entry.block_number;
    const auto tick_now = ::tickNow();

    SL_TRACE(logger_,
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
      const auto check = checkApproval(
          candidate_entry, approval_entry, status.required_tranches);
      const auto is_approved = approval::is_approved(
          check, math::sat_sub_unsigned(tick_now, kApprovalDelay));

      if (is_approved) {
        logger_->info("Candidate approved: candidate {}, block {}",
                      candidate_hash,
                      block_hash);
        const auto was_block_approved = block_entry.is_fully_approved();
        block_entry.mark_approved_by_hash(candidate_hash);
        const auto is_block_approved = block_entry.is_fully_approved();

        auto no_shows = known_no_shows(check);
        if (no_shows != 0) {
          // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
          metric_no_shows_total_->inc(no_shows);
        }

        if (is_block_approved && !was_block_approved) {
          notifyApproved(block_hash);
        }
      }
      _ = std::make_pair(is_approved, std::move(status));
      ae = approval_entry;
    }

    BOOST_ASSERT(_);
    BOOST_ASSERT(ae);
    auto &[is_approved, status] = *_;
    auto &approval_entry = ae->get();

    const auto was_approved = approval_entry.approved;
    const auto newly_approved = is_approved && !was_approved;
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

    if (is_approved && is_remote_approval(transition)) {
      for (const auto &[fork_block_hash, fork_approval_entry] :
           candidate_entry.block_assignments) {
        if (fork_block_hash == block_hash) {
          continue;
        }

        bool assigned_on_fork_block = false;
        if (validator_index) {
          assigned_on_fork_block =
              fork_approval_entry.is_assigned(*validator_index);
        }

        if (!wakeup_for(fork_block_hash, candidate_hash)
            && !fork_approval_entry.approved && assigned_on_fork_block) {
          auto opt_fork_block_entry = storedBlockEntries().get(fork_block_hash);
          if (!opt_fork_block_entry) {
            SL_TRACE(logger_,
                     "Failed to load block entry. (fork_block_hash={})",
                     fork_block_hash);
          } else {
            runScheduleWakeup(fork_block_hash,
                              opt_fork_block_entry->get().block_number,
                              candidate_hash,
                              tick_now + 1);
          }
        }
      }
    }

    if (approval::is_local_approval(transition) || newly_approved
        || (already_approved_by && !*already_approved_by)) {
      BOOST_ASSERT(storedCandidateEntries().get(candidate_hash)->get()
                   == candidate_entry);
    }
  }

  void ApprovalDistribution::scheduleTranche(
      const primitives::BlockHash &head, BlockImportedCandidates &&candidate) {
    /// this_thread_ context execution.
    BOOST_ASSERT(approval_thread_handler_->isInCurrentThread());
    SL_TRACE(logger_,
             "Imported new block {}:{} with candidates count {}",
             candidate.block_number,
             candidate.block_hash,
             candidate.imported_candidates.size());

    for (const auto &[c_hash, c_entry] : candidate.imported_candidates) {
      const auto &block_assignments = c_entry.block_assignments.at(head);
      if (block_assignments.our_assignment) {
        const auto our_tranche = block_assignments.our_assignment->tranche;
        const auto tick = our_tranche + candidate.block_tick;
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
      const primitives::BlockHash &block_hash,
      primitives::BlockNumber block_number,
      const CandidateHash &candidate_hash,
      Tick tick) {
    const auto ms_now = msNow();
    const auto ms_wakeup = tick * kTickDurationMs;
    const auto ms_wakeup_after = math::sat_sub_unsigned(ms_wakeup, ms_now);

    auto &target_block = active_tranches_[block_hash];
    SL_TRACE(logger_,
             "Scheduling wakeup. (block_hash={}, candidate_hash={}, "
             "block_number={}, tick={}, after={})",
             block_hash,
             candidate_hash,
             block_number,
             tick,
             ms_wakeup_after);

    auto handle = scheduler_->scheduleWithHandle(
        [wself{weak_from_this()}, block_hash, block_number, candidate_hash]() {
          if (auto self = wself.lock()) {
            BOOST_ASSERT(self->approval_thread_handler_->isInCurrentThread());
            if (auto target_block_it = self->active_tranches_.find(block_hash);
                target_block_it != self->active_tranches_.end()) {
              self->handleTranche(block_hash, block_number, candidate_hash);
            }
          }
        },
        std::chrono::milliseconds(ms_wakeup_after));
    target_block[candidate_hash].emplace_back(tick, std::move(handle));
  }

  bool ApprovalDistribution::wakeup_for(const primitives::BlockHash &block_hash,
                                        const CandidateHash &candidate_hash) {
    auto it = active_tranches_.find(block_hash);
    return it != active_tranches_.end() && it->second.contains(candidate_hash);
  }

  void ApprovalDistribution::handleTranche(
      const primitives::BlockHash &block_hash,
      primitives::BlockNumber block_number,
      const CandidateHash &candidate_hash) {
    BOOST_ASSERT(approval_thread_handler_->isInCurrentThread());

    auto opt_block_entry = storedBlockEntries().get(block_hash);
    auto opt_candidate_entry = storedCandidateEntries().get(candidate_hash);

    if (!opt_block_entry || !opt_candidate_entry) {
      SL_ERROR(logger_, "Block entry or candidate entry not exists.");
      return;
    }

    auto &block_entry = opt_block_entry->get();
    auto &candidate_entry = opt_candidate_entry->get();

    std::optional<runtime::SessionInfo> opt_session_info{};
    if (auto session_info_res = parachain_host_->session_info(
            block_entry.parent_hash, block_entry.session);
        session_info_res.has_value()) {
      opt_session_info = std::move(session_info_res.value());
    } else {
      logger_->warn(
          "Handle tranche. Session info runtime request failed. "
          "(block_hash={}, session_index={}, error={})",
          block_entry.parent_hash,
          block_entry.session,
          session_info_res.error());
      return;
    }

    if (!opt_session_info) {
      logger_->debug(
          "Can't obtain SessionInfo. (parent_hash={}, session_index={})",
          block_entry.parent_hash,
          block_entry.session);
      return;
    }

    runtime::SessionInfo &session_info = *opt_session_info;
    const auto block_tick =
        slotNumberToTick(config_.slot_duration_millis, block_entry.slot);
    const auto no_show_duration = slotNumberToTick(config_.slot_duration_millis,
                                                   session_info.no_show_slots);

    const auto tranche_now =
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
    const auto tta = tranchesToApprove(approval_entry,
                                       candidate_entry.approvals,
                                       tranche_now,
                                       block_tick,
                                       no_show_duration,
                                       session_info.needed_approvals);
    const auto should_trigger = shouldTriggerAssignment(
        approval_entry, candidate_entry, tta, tranche_now);
    const auto backing_group = approval_entry.backing_group;
    const auto &candidate_receipt = candidate_entry.candidate.get();

    ApprovalEntry::MaybeCert maybe_cert{};
    if (should_trigger) {
      maybe_cert = approval_entry.trigger_our_assignment(tickNow());
      BOOST_ASSERT(storedCandidateEntries().get(candidate_hash)->get()
                   == candidate_entry);
    }
    SL_TRACE(logger_,
             "Wakeup processed. (should trigger={}, cert={})",
             should_trigger,
             (bool)maybe_cert);

    if (maybe_cert) {
      const auto &[cert, val_index, tranche] = *maybe_cert;
      approval::IndirectAssignmentCertV2 indirect_cert{
          .block_hash = block_hash,
          .validator = val_index,
          .cert = cert.get(),
      };

      SL_TRACE(logger_,
               "Launching approval work. (candidate_hash={}, para_id={}, "
               "block_hash={})",
               candidate_hash,
               candidate_receipt.descriptor.para_id,
               block_hash);

      auto candidate_core_index = [&]() -> std::optional<CoreIndex> {
        for (const auto &[core_index, h] : block_entry.candidates) {
          if (candidate_hash == h) {
            return core_index;
          }
        }
        return std::nullopt;
      }();

      if (auto claimed_core_indices = get_assignment_core_indices(
              indirect_cert.cert.kind, candidate_hash, block_entry)) {
        if (auto claimed_candidate_indices = cores_to_candidate_indices(
                *claimed_core_indices, block_entry)) {
          // NOLINTNEXTLINE(cppcoreguidelines-init-variables)
          bool distribute_assignment;
          if (approval::count_ones(*claimed_candidate_indices) > 1) {
            distribute_assignment = !block_entry.mark_assignment_distributed(
                *claimed_candidate_indices);
          } else {
            distribute_assignment = true;
          }

          BOOST_ASSERT(storedBlockEntries().get(block_hash)->get()
                       == block_entry);
          runLaunchApproval(indirect_cert,
                            tranche,
                            block_hash,
                            *claimed_candidate_indices,
                            block_entry.session,
                            candidate_entry.candidate,
                            backing_group,
                            candidate_core_index,
                            distribute_assignment);

        } else {
          SL_WARN(logger_,
                  "Failed to create assignment bitfield. (block_hash={})",
                  block_hash);
        }
      } else {
        SL_WARN(logger_,
                "Cannot get assignment claimed core indices. "
                "(candidate_hash={}, block_hash={})",
                candidate_hash,
                block_hash);
      }
    }

    advance_approval_state(block_entry,
                           candidate_hash,
                           candidate_entry,
                           approval::WakeupProcessed{});
  }

  void ApprovalDistribution::unify_with_peer(
      StoreUnit<StorePair<Hash, DistribBlockEntry>> &entries,
      const libp2p::peer::PeerId &peer_id,
      const network::View &view,
      bool retry_known_blocks) {
    std::deque<network::vstaging::Assignment> assignments_to_send;
    std::deque<approval::IndirectSignedApprovalVoteV2> approvals_to_send;

    const auto view_finalized_number = view.finalized_number_;
    for (const auto &head : view.heads_) {
      primitives::BlockHash block = head;
      while (true) {
        auto opt_entry = entries.get(block);
        if (!opt_entry || opt_entry->get().number <= view_finalized_number) {
          break;
        }

        auto &entry = opt_entry->get();
        if (entry.known_by.contains(peer_id) and !retry_known_blocks) {
          break;
        }

        auto &peer_knowledge = entry.known_by[peer_id];
        for (auto &[_, approval_entry] : entry.approval_entries) {
          [[maybe_unused]] const auto &required_routing =
              approval_entry.routing_info.required_routing;
          [[maybe_unused]] auto &routing_info = approval_entry.routing_info;

          auto peer_filter = [&](const libp2p::peer::PeerId &) {
            /// TODO(iceseer): check topology
            return true;
          };

          if (!peer_filter(peer_id)) {
            continue;
          }

          const auto assignment_message = approval_entry.get_assignment();
          const auto approval_messages = approval_entry.get_approvals();
          const auto [assignment_knowledge, message_kind] =
              approval_entry.create_assignment_knowledge(block);

          if (!peer_knowledge.contains(assignment_knowledge, message_kind)) {
            peer_knowledge.sent.insert(assignment_knowledge, message_kind);
            assignments_to_send.emplace_back(network::vstaging::Assignment{
                .indirect_assignment_cert = assignment_message.first,
                .candidate_bitfield = assignment_message.second,
            });
          }

          for (const auto &approval_message : approval_messages) {
            auto approval_knowledge =
                approval::PeerKnowledge::generate_approval_key(
                    approval_message);

            if (!peer_knowledge.contains(approval_knowledge.first,
                                         approval_knowledge.second)) {
              approvals_to_send.emplace_back(approval_message);
              peer_knowledge.sent.insert(approval_knowledge.first,
                                         approval_knowledge.second);
            }
          }
        }

        block = entry.parent_hash;
      }
    }

    if (!assignments_to_send.empty()) {
      SL_TRACE(logger_,
               "Sending assignments to unified peer. (peer id={}, count={})",
               peer_id,
               assignments_to_send.size());
      send_assignments_batched(std::move(assignments_to_send), peer_id);
    }

    if (!approvals_to_send.empty()) {
      SL_TRACE(logger_,
               "Sending approvals to unified peer. (peer id={}, count={})",
               peer_id,
               approvals_to_send.size());
      send_approvals_batched(std::move(approvals_to_send), peer_id);
    }
  }

  primitives::BlockInfo ApprovalDistribution::approvedAncestor(
      const primitives::BlockInfo &min,
      const primitives::BlockInfo &max) const {
    if (not approval_thread_handler_->isInCurrentThread()) {
      std::promise<primitives::BlockInfo> promise;
      auto future = promise.get_future();
      approval_thread_handler_->execute(
          libp2p::SharedFn{[&, promise{std::move(promise)}]() mutable {
            promise.set_value(approvedAncestor(min, max));
          }});
      return future.get();
    }

    if (max.number <= min.number) {
      return min;
    }
    auto count = max.number - min.number;
    auto chain_res = block_tree_->getDescendingChainToBlock(max.hash, count);
    if (not chain_res) {
      return min;
    }
    auto &chain = chain_res.value();
    BOOST_ASSERT(not chain.empty() and chain[0] == max.hash);
    BOOST_ASSERT(chain.size() == count);
    std::ranges::reverse(chain);
    BOOST_ASSERT(chain[0] != min.hash);
    primitives::BlockInfo result = min;
    for (auto &hash : chain) {
      auto entry = storedBlockEntries().get(hash);
      if (not entry or not entry->get().is_fully_approved()) {
        break;
      }
      result = {result.number + 1, hash};
    }
    return result;
  }
}  // namespace kagome::parachain
