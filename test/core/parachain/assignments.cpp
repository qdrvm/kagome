/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/key_store/key_store_impl.hpp"

#include <filesystem>
#include <mock/libp2p/crypto/random_generator_mock.hpp>
#include <string_view>

#include <gmock/gmock.h>

#include "common/blob.hpp"
#include "common/visitor.hpp"
#include "crypto/bandersnatch/bandersnatch_provider_impl.hpp"
#include "crypto/bandersnatch/vrf.hpp"
#include "crypto/bip39/impl/bip39_provider_impl.hpp"
#include "crypto/ecdsa/ecdsa_provider_impl.hpp"
#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"
#include "crypto/sr25519_types.hpp"
#include "crypto/vrf/vrf_provider_impl.hpp"
#include "log/logger.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "parachain/approval/approval_distribution.hpp"
#include "parachain/approval/approval_distribution_error.hpp"
#include "primitives/transcript.hpp"
#include "testutil/prepare_loggers.hpp"
#include "testutil/storage/base_fs_test.hpp"

using kagome::common::Blob;
using kagome::common::Buffer;
using namespace kagome::crypto;

static auto assignments_directory =
    kagome::filesystem::temp_directory_path() / "assignments_test";

struct AssignmentsTest : public test::BaseFS_Test {
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  static kagome::log::Logger &log() {
    static auto logger = kagome::log::createLogger("test");
    return logger;
  }

  VRFProviderImpl vrf_provider_;

  AssignmentsTest()
      : BaseFS_Test(assignments_directory),
        vrf_provider_(std::make_shared<BoostRandomGenerator>()) {}

  void SetUp() override {}

  template <size_t N>
  auto assignment_keys_plus_random(std::shared_ptr<KeyStore> &cs,
                                   const char *const (&accounts)[N],
                                   size_t random) {
    std::vector<Sr25519PublicKey> keys;
    for (const auto &acc : accounts) {
      auto keypair =
          cs->sr25519()
              .generateKeypair(KeyTypes::ASSIGNMENT, std::string_view{acc})
              .value();
      keys.push_back(keypair.public_key);
    }
    for (size_t ix = 0ull; ix < random; ++ix) {
      auto seed = std::to_string(ix);
      auto keypair =
          cs->sr25519()
              .generateKeypair(KeyTypes::ASSIGNMENT, std::string_view{seed})
              .value();
      keys.push_back(keypair.public_key);
    }
    return keys;
  }

  auto create_crypto_store() {
    auto hasher = std::make_shared<HasherImpl>();
    auto csprng = std::make_shared<BoostRandomGenerator>();
    auto ecdsa_provider = std::make_shared<EcdsaProviderImpl>(hasher);
    auto ed25519_provider = std::make_shared<Ed25519ProviderImpl>(hasher);
    auto sr25519_provider = std::make_shared<Sr25519ProviderImpl>();
    auto bandersnatch_provider =
        std::make_shared<BandersnatchProviderImpl>(hasher);

    auto pbkdf2_provider = std::make_shared<Pbkdf2ProviderImpl>();
    auto bip39_provider = std::make_shared<Bip39ProviderImpl>(
        std::move(pbkdf2_provider),
        std::make_shared<libp2p::crypto::random::CSPRNGMock>(),
        hasher);

    auto keystore_path = kagome::filesystem::path(__FILE__).parent_path()
                       / "subkey_keys" / "keystore";
    std::shared_ptr key_file_storage =
        kagome::crypto::KeyFileStorage::createAt(keystore_path).value();
    KeyStore::Config config{keystore_path};
    return std::make_shared<KeyStore>(
        std::make_unique<KeySuiteStoreImpl<Sr25519Provider>>(
            std::move(sr25519_provider),
            bip39_provider,
            csprng,
            key_file_storage),
        std::make_unique<KeySuiteStoreImpl<Ed25519Provider>>(
            ed25519_provider,  //
            bip39_provider,
            csprng,
            key_file_storage),
        std::make_unique<KeySuiteStoreImpl<EcdsaProvider>>(
            std::move(ecdsa_provider),
            bip39_provider,
            csprng,
            key_file_storage),
        std::make_unique<KeySuiteStoreImpl<BandersnatchProvider>>(
            std::move(bandersnatch_provider),
            bip39_provider,
            csprng,
            key_file_storage),
        ed25519_provider,
        std::make_shared<kagome::application::AppStateManagerMock>(),
        config);
  }

  std::vector<std::vector<kagome::parachain::ValidatorIndex>> basic_groups(
      size_t n_validators, size_t n_cores) {
    std::vector<std::vector<kagome::parachain::ValidatorIndex>> groups;

    // Calculate the number of validators per group
    size_t validators_per_group = n_validators / n_cores;
    size_t remaining_validators = n_validators % n_cores;

    // Distribute validators across groups
    size_t validator_index = 0;
    for (size_t i = 0; i < n_cores; ++i) {
      std::vector<kagome::parachain::ValidatorIndex> group;
      size_t group_size =
          validators_per_group + (i < remaining_validators ? 1 : 0);

      for (size_t j = 0; j < group_size; ++j) {
        group.push_back(
            static_cast<kagome::parachain::ValidatorIndex>(validator_index++));
      }

      groups.push_back(group);
    }

    return groups;
  }

  kagome::crypto::VRFOutput garbage_vrf_signature() {
    // Create a transcript with the same context as Rust version
    kagome::primitives::Transcript transcript;
    transcript.initialize(
        {'t', 'e', 's', 't', '-', 'g', 'a', 'r', 'b', 'a', 'g', 'e'});

    // Get Alice's keypair
    SecureBuffer<> seed_buf(Sr25519Seed::size(), 42);
    auto seed = Sr25519Seed::from(std::move(seed_buf)).value();

    auto alice_keypair = vrf_provider_.generateKeypair();

    auto result = vrf_provider_.signTranscript(transcript, alice_keypair);

    if (not result) {
      throw std::runtime_error("Failed to create garbage VRF signature");
    }

    return result.value();
  }

  // Helper function to check mutated assignments
  void check_mutated_assignments(
      const std::function<std::optional<bool>(
          scale::BitVec &,
          kagome::parachain::approval::AssignmentCertV2 &,
          std::vector<kagome::network::GroupIndex> &,
          kagome::network::GroupIndex,
          kagome::network::ValidatorIndex,
          kagome::runtime::SessionInfo &)> &check_fn) {
    auto cs = create_crypto_store();
    auto asgn_keys =
        assignment_keys_plus_random(cs, {"//Alice", "//Bob", "//Charlie"}, 0ull);

    ::RelayVRFStory vrf_story;
    ::memset(vrf_story.data, uint8_t{42}, sizeof(vrf_story.data));

    kagome::runtime::SessionInfo si;
    for (const auto &a : asgn_keys) {
      si.assignment_keys.emplace_back(a);
    }

    si.validator_groups = basic_groups(200, 100);
    si.n_cores = 100;
    si.zeroth_delay_tranche_width = 10;
    si.relay_vrf_modulo_samples = 15;
    si.n_delay_tranches = 40;

    kagome::parachain::ApprovalDistribution::CandidateIncludedList leaving_cores{};
    for (size_t i = 0; i < 100; ++i) {
      leaving_cores.emplace_back(std::make_tuple(
          kagome::parachain::ApprovalDistribution::HashedCandidateReceipt{
              kagome::network::CandidateReceipt{}},
          (kagome::parachain::CoreIndex)i,
          (kagome::parachain::GroupIndex)((i + 25) % 100)));
    }

    auto assignments =
        kagome::parachain::ApprovalDistribution::compute_assignments(
            cs, si, vrf_story, leaving_cores, false, log());
    auto assignments2 =
        kagome::parachain::ApprovalDistribution::compute_assignments(
            cs, si, vrf_story, leaving_cores, true, log());
    for (auto &[core, assignment] : assignments2) {
      assignments[core] = std::move(assignment);
    }

    size_t counted = 0;
    for (auto &[core, assignment] : assignments) {
      scale::BitVec cores;
      kagome::visit_in_place(
          assignment.cert.kind,
          [&](const kagome::parachain::approval::RelayVRFModuloCompact
                  &compact) { cores = compact.core_bitfield; },
          [&](const kagome::parachain::approval::RelayVRFModulo &modulo) {
            cores.bits.resize(core + 1);
            cores.bits[core] = true;
          },
          [&](const kagome::parachain::approval::RelayVRFDelay &delay) {
            cores.bits.resize(delay.core_index + 1);
            cores.bits[delay.core_index] = true;
          });

      std::vector<kagome::network::GroupIndex> groups;
      for (size_t i = 0; i < cores.bits.size(); ++i) {
        if (cores.bits[i]) {
          groups.emplace_back((i + 25) % 100);
        }
      }

      std::optional<bool> expected = check_fn(cores,
                                              assignment.cert,
                                              groups,
                                              0,
                                              0,
                                              si);  // own_group, val_index
      if (!expected) {
        continue;
      }
      ++counted;

      auto is_good = kagome::parachain::checkAssignmentCert(
                         cores, 0, si, vrf_story, assignment.cert, groups)
                         .has_value();

      ASSERT_EQ(expected.value(), is_good);
    }
    ASSERT_GT(counted, 0);
  }
};

/**
 * There should be no assignments in no cores available.
 */
TEST_F(AssignmentsTest, succeeds_empty_for_0_cores) {
  auto cs = create_crypto_store();
  auto asgn_keys =
      assignment_keys_plus_random(cs, {"//Alice", "//Bob", "//Charlie"}, 0ull);

  ::RelayVRFStory vrf_story;
  ::memset(vrf_story.data, 42, sizeof(vrf_story.data));

  kagome::runtime::SessionInfo si;
  for (const auto &a : asgn_keys) {
    si.assignment_keys.emplace_back(a);
  }

  si.n_cores = 0;
  si.zeroth_delay_tranche_width = 10;
  si.relay_vrf_modulo_samples = 10;
  si.n_delay_tranches = 40;

  kagome::parachain::ApprovalDistribution::CandidateIncludedList
      leaving_cores{};

  auto assignments =
      kagome::parachain::ApprovalDistribution::compute_assignments(
          cs, si, vrf_story, leaving_cores, false, log());

  ASSERT_EQ(assignments.size(), 0ull);
}

/**
 * There should be an assignment for a 1 core.
 */
TEST_F(AssignmentsTest, assign_to_nonzero_core) {
  auto cs = create_crypto_store();
  auto asgn_keys =
      assignment_keys_plus_random(cs, {"//Alice", "//Bob", "//Charlie"}, 0ull);

  ::RelayVRFStory vrf_story;
  ::memset(vrf_story.data, 42, sizeof(vrf_story.data));

  kagome::runtime::SessionInfo si;
  for (const auto &a : asgn_keys) {
    si.assignment_keys.emplace_back(a);
  }

  si.validator_groups.emplace_back(
      std::vector<kagome::parachain::ValidatorIndex>{0});
  si.validator_groups.emplace_back(
      std::vector<kagome::parachain::ValidatorIndex>{1, 2});
  si.n_cores = 2;
  si.zeroth_delay_tranche_width = 10;
  si.relay_vrf_modulo_samples = 10;
  si.n_delay_tranches = 40;

  kagome::parachain::ApprovalDistribution::CandidateIncludedList leaving_cores =
      {std::make_tuple(
           kagome::parachain::ApprovalDistribution::HashedCandidateReceipt{
               kagome::network::CandidateReceipt{}},
           static_cast<kagome::parachain::CoreIndex>(0),
           static_cast<kagome::parachain::GroupIndex>(0)),
       std::make_tuple(
           kagome::parachain::ApprovalDistribution::HashedCandidateReceipt{
               kagome::network::CandidateReceipt{}},
           static_cast<kagome::parachain::CoreIndex>(1),
           static_cast<kagome::parachain::GroupIndex>(1))};
  auto assignments =
      kagome::parachain::ApprovalDistribution::compute_assignments(
          cs, si, vrf_story, leaving_cores, false, log());

  ASSERT_EQ(assignments.size(), 1ull);

  auto it = assignments.find(1ull);
  ASSERT_TRUE(it != assignments.end());

  const kagome::parachain::ApprovalDistribution::OurAssignment &our_assignment =
      it->second;
  ASSERT_EQ(our_assignment.tranche, 0ull);
  ASSERT_EQ(our_assignment.validator_index, 0);
  ASSERT_EQ(our_assignment.triggered, false);

  ASSERT_TRUE(
      kagome::is_type<const kagome::parachain::approval::RelayVRFModulo>(
          our_assignment.cert.kind));
  if (auto k =
          kagome::if_type<const kagome::parachain::approval::RelayVRFModulo>(
              our_assignment.cert.kind)) {
    ASSERT_EQ(k->get().sample, 2);
  }

  const uint8_t
      vrf_output[kagome::crypto::constants::sr25519::vrf::OUTPUT_SIZE] = {
          228, 179, 248, 78,  77,  169, 23,  184, 138, 204, 148,
          183, 13,  41,  176, 163, 162, 6,   237, 158, 220, 225,
          97,  251, 51,  144, 207, 239, 189, 2,   7,   66};
  ASSERT_EQ(0,
            memcmp(&our_assignment.cert.vrf.output,
                   vrf_output,
                   kagome::crypto::constants::sr25519::vrf::OUTPUT_SIZE));
}

/**
 * There should be an assignment for a 1 core.
 */
TEST_F(AssignmentsTest, assignments_produced_for_non_backing) {
  auto cs = create_crypto_store();
  auto asgn_keys =
      assignment_keys_plus_random(cs, {"//Alice", "//Bob", "//Charlie"}, 0ull);

  ::RelayVRFStory vrf_story;
  ::memset(vrf_story.data, 42, sizeof(vrf_story.data));

  kagome::runtime::SessionInfo si;
  for (const auto &a : asgn_keys) {
    si.assignment_keys.emplace_back(a);
  }

  si.validator_groups.emplace_back(
      std::vector<kagome::parachain::ValidatorIndex>{0});
  si.validator_groups.emplace_back(
      std::vector<kagome::parachain::ValidatorIndex>{1, 2});
  si.n_cores = 2;
  si.zeroth_delay_tranche_width = 10;
  si.relay_vrf_modulo_samples = 10;
  si.n_delay_tranches = 40;

  kagome::parachain::ApprovalDistribution::CandidateIncludedList leaving_cores =
      {std::make_tuple(
           kagome::parachain::ApprovalDistribution::HashedCandidateReceipt{
               kagome::network::CandidateReceipt{}},
           (kagome::parachain::CoreIndex)0,
           (kagome::parachain::GroupIndex)1),
       std::make_tuple(
           kagome::parachain::ApprovalDistribution::HashedCandidateReceipt{
               kagome::network::CandidateReceipt{}},
           (kagome::parachain::CoreIndex)1,
           (kagome::parachain::GroupIndex)0)};
  auto assignments =
      kagome::parachain::ApprovalDistribution::compute_assignments(
          cs, si, vrf_story, leaving_cores, false, log());

  ASSERT_EQ(assignments.size(), 1ull);

  auto it = assignments.find(0ull);
  ASSERT_TRUE(it != assignments.end());

  const kagome::parachain::ApprovalDistribution::OurAssignment &our_assignment =
      it->second;
  ASSERT_EQ(our_assignment.tranche, 0ull);
  ASSERT_EQ(our_assignment.validator_index, 0);
  ASSERT_EQ(our_assignment.triggered, false);

  ASSERT_TRUE(
      kagome::is_type<const kagome::parachain::approval::RelayVRFModulo>(
          our_assignment.cert.kind));
  if (auto k =
          kagome::if_type<const kagome::parachain::approval::RelayVRFModulo>(
              our_assignment.cert.kind)) {
    ASSERT_EQ(k->get().sample, 0);
  }

  const uint8_t
      vrf_output[kagome::crypto::constants::sr25519::vrf::OUTPUT_SIZE] = {
          34,  247, 30,  171, 146, 67, 68,  83,  108, 206, 61,
          154, 115, 28,  180, 81,  28, 90,  68,  166, 49,  220,
          157, 41,  235, 223, 152, 45, 190, 202, 216, 39};
  ASSERT_EQ(0,
            memcmp(&our_assignment.cert.vrf.output,
                   vrf_output,
                   kagome::crypto::constants::sr25519::vrf::OUTPUT_SIZE));
}

TEST_F(AssignmentsTest, check_rejects_modulo_bad_vrf) {
  check_mutated_assignments(
      [&](scale::BitVec &cores,
          kagome::parachain::approval::AssignmentCertV2 &cert,
          std::vector<kagome::network::GroupIndex> &groups,
          kagome::network::GroupIndex own_group,
          kagome::network::ValidatorIndex val_index,
          const kagome::runtime::SessionInfo &config) -> std::optional<bool> {
        auto vrf_signature = garbage_vrf_signature();
        if (auto k =
                kagome::if_type<kagome::parachain::approval::RelayVRFModulo>(
                    cert.kind)) {
          cert.vrf = vrf_signature;
          return false;
        } else if (auto k = kagome::if_type<
                       kagome::parachain::approval::RelayVRFModuloCompact>(
                       cert.kind)) {
          cert.vrf = vrf_signature;
          return false;
        } else {
          return std::nullopt;
        }
      });
}

TEST_F(AssignmentsTest, check_rejects_modulo_sample_out_of_bounds) {
  check_mutated_assignments(
      [&](scale::BitVec &cores,
          kagome::parachain::approval::AssignmentCertV2 &cert,
          std::vector<kagome::network::GroupIndex> &groups,
          kagome::network::GroupIndex own_group,
          kagome::network::ValidatorIndex val_index,
          kagome::runtime::SessionInfo &config) -> std::optional<bool> {
        if (auto k =
                kagome::if_type<kagome::parachain::approval::RelayVRFModulo>(
                    cert.kind)) {
          config.relay_vrf_modulo_samples = k->get().sample;
          return false;
        } else if (auto k = kagome::if_type<
                       kagome::parachain::approval::RelayVRFModuloCompact>(
                       cert.kind)) {
          return true;
        } else {
          return std::nullopt;
        }
      });
}
