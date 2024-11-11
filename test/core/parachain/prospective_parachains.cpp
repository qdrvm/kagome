/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/prospective_parachains/prospective_parachains.hpp"
#include <map>
#include <vector>
#include "core/parachain/parachain_test_harness.hpp"
#include "parachain/validator/parachain_processor.hpp"

using namespace kagome::parachain;
namespace runtime = kagome::runtime;

class ProspectiveParachainsTest : public ProspectiveParachainsTestHarness {
  void SetUp() override {
    ProspectiveParachainsTestHarness::SetUp();
    parachain_api_ = std::make_shared<runtime::ParachainHostMock>();
    prospective_parachain_ = std::make_shared<ProspectiveParachains>(
        hasher_, parachain_api_, block_tree_);
  }

  void TearDown() override {
    prospective_parachain_.reset();
    parachain_api_.reset();
    ProspectiveParachainsTestHarness::TearDown();
  }

 public:
  std::shared_ptr<runtime::ParachainHostMock> parachain_api_;
  std::shared_ptr<ProspectiveParachains> prospective_parachain_;

  struct TestState {
    std::map<CoreIndex, std::vector<ParachainId>> claim_queue = {
        {CoreIndex(0), {ParachainId(1)}}, {CoreIndex(1), {ParachainId(2)}}};
    uint32_t runtime_api_version = CLAIM_QUEUE_RUNTIME_REQUIREMENT;
    ValidationCodeHash validation_code_hash = fromNumber(42);

    void set_runtime_api_version(uint32_t version) {
      runtime_api_version = version;
    }
  };

  struct PerParaData {
    BlockNumber min_relay_parent;
    HeadData head_data;
    std::vector<fragment::CandidatePendingAvailability> pending_availability;

    PerParaData(BlockNumber min_relay_parent_, const HeadData &head_data_)
        : min_relay_parent{min_relay_parent_}, head_data{head_data_} {}

    PerParaData(
        BlockNumber min_relay_parent_,
        const HeadData &head_data_,
        const std::vector<fragment::CandidatePendingAvailability> &pending_)
        : min_relay_parent{min_relay_parent_},
          head_data{head_data_},
          pending_availability{pending_} {}
  };

  struct TestLeaf {
    BlockNumber number;
    Hash hash;
    std::vector<std::pair<ParachainId, PerParaData>> para_data;

    std::reference_wrapper<const PerParaData> paraData(
        ParachainId para_id) const {
      for (const auto &[para, per_data] : para_data) {
        if (para == para_id) {
          return {per_data};
        }
      }
      UNREACHABLE;
    }
  };

  fragment::Constraints dummy_constraints(
      BlockNumber min_relay_parent_number,
      std::vector<BlockNumber> valid_watermarks,
      const HeadData &required_parent,
      const ValidationCodeHash &validation_code_hash) {
    return fragment::Constraints{
        .min_relay_parent_number = min_relay_parent_number,
        .max_pov_size = MAX_POV_SIZE,
        .max_code_size = 1000000,
        .ump_remaining = 10,
        .ump_remaining_bytes = 1000,
        .max_ump_num_per_candidate = 10,
        .dmp_remaining_messages = {},
        .hrmp_inbound =
            fragment::InboundHrmpLimitations{
                .valid_watermarks = valid_watermarks,
            },
        .hrmp_channels_out = {},
        .max_hrmp_num_per_candidate = 0,
        .required_parent = required_parent,
        .validation_code_hash = validation_code_hash,
        .upgrade_restriction = {},
        .future_validation_code = {},
    };
  }

  static Hash get_parent_hash(const Hash &parent) {
    const auto val = *(uint8_t *)&parent[0];
    return fromNumber(val + 1);
  }

  void handle_leaf_activation_2(
      const network::ExView &update,
      const TestLeaf &leaf,
      const TestState &test_state,
      const fragment::AsyncBackingParams &async_backing_params) {
    const auto &[number, hash, para_data] = leaf;
    const auto &header = update.new_head;

    EXPECT_CALL(*parachain_api_, staging_async_backing_params(hash))
        .WillRepeatedly(Return(outcome::success(async_backing_params)));

    EXPECT_CALL(*parachain_api_, runtime_api_version(hash))
        .WillRepeatedly(
            Return(outcome::success(test_state.runtime_api_version)));

    if (test_state.runtime_api_version < CLAIM_QUEUE_RUNTIME_REQUIREMENT) {
      std::vector<runtime::CoreState> cores;
      for (const auto &[_, paras] : test_state.claim_queue) {
        cores.emplace_back(runtime::ScheduledCore{
            .para_id = paras[0],
            .collator = std::nullopt,
        });
      }
      EXPECT_CALL(*parachain_api_, availability_cores(hash))
          .WillRepeatedly(Return(outcome::success(cores)));
    } else {
      EXPECT_CALL(*parachain_api_, claim_queue(hash))
          .WillRepeatedly(Return(outcome::success(test_state.claim_queue)));
    }

    EXPECT_CALL(*block_tree_, getBlockHeader(hash))
        .WillRepeatedly(Return(header));

    const BlockNumber min_min = [&, number = number]() -> BlockNumber {
      std::optional<BlockNumber> min_min;
      for (const auto &[_, data] : leaf.para_data) {
        min_min = (min_min ? std::min(*min_min, data.min_relay_parent)
                           : data.min_relay_parent);
      }
      if (min_min) {
        return *min_min;
      }
      return number;
    }();

    const auto ancestry_len = number - min_min;
    std::vector<Hash> ancestry_hashes;
    std::vector<BlockNumber> ancestry_numbers;

    Hash d = hash;
    for (BlockNumber x = 1; x <= ancestry_len; ++x) {
      d = get_parent_hash(d);
      ancestry_hashes.emplace_back(d);
      ancestry_numbers.push_back(number - x);
    }
    ASSERT_EQ(ancestry_hashes.size(), ancestry_numbers.size());

    if (ancestry_len > 0) {
      EXPECT_CALL(*block_tree_,
                  getDescendingChainToBlock(hash, ALLOWED_ANCESTRY_LEN + 1))
          .WillRepeatedly(Return(ancestry_hashes));
      EXPECT_CALL(*parachain_api_, session_index_for_child(hash))
          .WillRepeatedly(Return(1));
    }

    std::unordered_set<Hash> used_relay_parents;
    for (size_t i = 0; i < ancestry_hashes.size(); ++i) {
      const auto &h_ = ancestry_hashes[i];
      const auto &n_ = ancestry_numbers[i];

      if (!used_relay_parents.contains(h_)) {
        BlockHeader h{
            .number = n_,
            .parent_hash = get_parent_hash(h_),
            .state_root = {},
            .extrinsics_root = {},
            .digest = {},
            .hash_opt = {},
        };
        EXPECT_CALL(*block_tree_, getBlockHeader(h_)).WillRepeatedly(Return(h));
        EXPECT_CALL(*parachain_api_, session_index_for_child(h_))
            .WillRepeatedly(Return(outcome::success(1)));
        used_relay_parents.emplace(h_);
      }
    }

    std::unordered_set<ParachainId> paras;
    for (const auto &[_, values] : test_state.claim_queue) {
      for (const auto &value : values) {
        paras.emplace(value);
      }
    }

    for (auto it = paras.begin(); it != paras.end(); ++it) {
      const auto para_id = *it;
      const auto &[min_relay_parent, head_data, pending_availability] =
          leaf.paraData(para_id).get();

      const auto constraints =
          dummy_constraints(min_relay_parent,
                            {number},
                            head_data,
                            test_state.validation_code_hash);
      const fragment::BackingState backing_state{
          .constraints = constraints,
          .pending_availability = pending_availability,
      };
      EXPECT_CALL(*parachain_api_, staging_para_backing_state(hash, para_id))
          .WillRepeatedly(Return(backing_state));

      for (const auto &pending : pending_availability) {
        if (!used_relay_parents.contains(pending.descriptor.relay_parent)) {
          BlockHeader h{
              .number = pending.relay_parent_number,
              .parent_hash = get_parent_hash(pending.descriptor.relay_parent),
              .state_root = {},
              .extrinsics_root = {},
              .digest = {},
              .hash_opt = {},
          };
          EXPECT_CALL(*block_tree_,
                      getBlockHeader(pending.descriptor.relay_parent))
              .WillRepeatedly(Return(h));
          used_relay_parents.emplace(pending.descriptor.relay_parent);
        }
      }
    }

    ASSERT_OUTCOME_SUCCESS_TRY(
        prospective_parachain_->onActiveLeavesUpdate(network::ExViewRef{
            .new_head = {update.new_head},
            .lost = update.lost,
        }));

    auto resp = prospective_parachain_->answerMinimumRelayParentsRequest(hash);
    std::sort(resp.begin(), resp.end(), [](const auto &l, const auto &r) {
      return l.first < r.first;
    });

    std::vector<std::pair<ParachainId, BlockNumber>> mrp_response;
    for (const auto &[pid, ppd] : para_data) {
      mrp_response.emplace_back(pid, ppd.min_relay_parent);
    }
    ASSERT_EQ(resp, mrp_response);
  }

  void activate_leaf(const TestLeaf &leaf,
                     const TestState &test_state,
                     const fragment::AsyncBackingParams &async_backing_params) {
    const auto &[number, hash, para_data] = leaf;
    BlockHeader header{
        .number = number,
        .parent_hash = get_parent_hash(hash),
        .state_root = {},
        .extrinsics_root = {},
        .digest = {},
        .hash_opt = {},
    };

    network::ExView update{
        .view = {},
        .new_head = header,
        .lost = {},
    };
    update.new_head.hash_opt = hash;
    handle_leaf_activation_2(update, leaf, test_state, async_backing_params);
  }
};

TEST_F(ProspectiveParachainsTest,
       should_do_no_work_if_async_backing_disabled_for_leaf) {
  const auto hash = fromNumber(130);
  network::ExView update{
      .view = {},
      .new_head =
          BlockHeader{
              .number = 1,
              .parent_hash = get_parent_hash(hash),
              .state_root = {},
              .extrinsics_root = {},
              .digest = {},
              .hash_opt = {},
          },
      .lost = {},
  };
  update.new_head.hash_opt = hash;

  EXPECT_CALL(*parachain_api_, staging_async_backing_params(hash))
      .WillRepeatedly(
          Return(outcome::failure(ParachainProcessorImpl::Error::NO_STATE)));

  std::ignore = prospective_parachain_->onActiveLeavesUpdate(network::ExViewRef{
      .new_head = {update.new_head},
      .lost = update.lost,
  });
  ASSERT_TRUE(prospective_parachain_->view().active_leaves.empty());
}
