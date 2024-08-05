/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/key_store/key_store_impl.hpp"

#include <filesystem>
#include <string_view>

#include <gmock/gmock.h>

#include "common/blob.hpp"
#include "common/visitor.hpp"
#include "crypto/bandersnatch/bandersnatch_provider_impl.hpp"
#include "crypto/bip39/impl/bip39_provider_impl.hpp"
#include "crypto/ecdsa/ecdsa_provider_impl.hpp"
#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"
#include "crypto/sr25519_types.hpp"
#include "log/logger.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "parachain/approval/approval_distribution.hpp"
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

  AssignmentsTest() : BaseFS_Test(assignments_directory) {}

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
    auto bip39_provider =
        std::make_shared<Bip39ProviderImpl>(std::move(pbkdf2_provider), hasher);

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
