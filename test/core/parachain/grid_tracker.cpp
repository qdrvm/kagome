/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/backing/grid_tracker.hpp"
#include "core/parachain/parachain_test_harness.hpp"

using namespace kagome::parachain::grid;

class GridTrackerTest : public ProspectiveParachainsTestHarness {
  void SetUp() override {
    ProspectiveParachainsTestHarness::SetUp();
  }

  void TearDown() override {
    ProspectiveParachainsTestHarness::TearDown();
  }

 public:
  Groups dummy_groups(size_t group_size) {
    std::vector<std::vector<ValidatorIndex>> groups;
    {
      std::vector<ValidatorIndex> group;
      for (size_t i = 0; i < group_size; ++i) {
        group.emplace_back(ValidatorIndex(i));
      }
      groups.emplace_back(group);
    }
    return Groups(std::move(groups), 2);
  }

  StatementFilter create_filter(const std::vector<bool> (&flags)[2]) {
    StatementFilter statement_filter;
    statement_filter.seconded_in_group = scale::BitVec{
        .bits = flags[0],
    };
    statement_filter.validated_in_group = scale::BitVec{
        .bits = flags[1],
    };
    return statement_filter;
  }
};

TEST_F(GridTrackerTest, reject_disallowed_manifest) {
  GridTracker tracker;

  SessionTopologyView session_topology = {View{
      .sending = {},
      .receiving = {ValidatorIndex(0)},
  }};

  const auto groups = dummy_groups(3);

  const auto candidate_hash = fromNumber(42);
  ASSERT_EQ(groups.get_size_and_backing_threshold(GroupIndex(0)),
            std::make_tuple(size_t(3), size_t(2)));

  // Known group, disallowed receiving validator.
  ASSERT_EQ(
      tracker
          .import_manifest(session_topology,
                           groups,
                           candidate_hash,
                           3,
                           ManifestSummary{
                               .claimed_parent_hash = fromNumber(0),
                               .claimed_group_index = GroupIndex(0),
                               .statement_knowledge = create_filter(
                                   {{false, true, false}, {true, false, true}}),
                           },
                           ManifestKind::Full,
                           ValidatorIndex(1))
          .error(),
      GridTracker::Error::DISALLOWED_DIRECTION);

  // Unknown group
  ASSERT_EQ(
      tracker
          .import_manifest(session_topology,
                           groups,
                           candidate_hash,
                           3,
                           ManifestSummary{
                               .claimed_parent_hash = fromNumber(0),
                               .claimed_group_index = GroupIndex(1),
                               .statement_knowledge = create_filter(
                                   {{false, true, false}, {true, false, true}}),
                           },
                           ManifestKind::Full,
                           ValidatorIndex(0))
          .error(),
      GridTracker::Error::DISALLOWED_GROUP_INDEX);
}

TEST_F(GridTrackerTest, reject_malformed_wrong_group_size) {
  GridTracker tracker;

  SessionTopologyView session_topology = {View{
      .sending = {},
      .receiving = {ValidatorIndex(0)},
  }};

  const auto groups = dummy_groups(3);

  const auto candidate_hash = fromNumber(42);
  ASSERT_EQ(groups.get_size_and_backing_threshold(GroupIndex(0)),
            std::make_tuple(size_t(3), size_t(2)));

  ASSERT_EQ(tracker
                .import_manifest(
                    session_topology,
                    groups,
                    candidate_hash,
                    3,
                    ManifestSummary{
                        .claimed_parent_hash = fromNumber(0),
                        .claimed_group_index = GroupIndex(0),
                        .statement_knowledge = create_filter(
                            {{false, true, false, true}, {true, false, true}}),
                    },
                    ManifestKind::Full,
                    ValidatorIndex(0))
                .error(),
            GridTracker::Error::MALFORMED_REMOTE_KNOWLEDGE_LEN);

  ASSERT_EQ(tracker
                .import_manifest(
                    session_topology,
                    groups,
                    candidate_hash,
                    3,
                    ManifestSummary{
                        .claimed_parent_hash = fromNumber(0),
                        .claimed_group_index = GroupIndex(0),
                        .statement_knowledge = create_filter(
                            {{false, true, false}, {true, false, true, false}}),
                    },
                    ManifestKind::Full,
                    ValidatorIndex(0))
                .error(),
            GridTracker::Error::MALFORMED_REMOTE_KNOWLEDGE_LEN);
}

TEST_F(GridTrackerTest, reject_malformed_no_seconders) {
  GridTracker tracker;
  SessionTopologyView session_topology = {View{
      .sending = {},
      .receiving = {ValidatorIndex(0)},
  }};

  const auto groups = dummy_groups(3);
  const auto candidate_hash = fromNumber(42);
  ASSERT_EQ(groups.get_size_and_backing_threshold(GroupIndex(0)),
            std::make_tuple(size_t(3), size_t(2)));

  ASSERT_EQ(
      tracker
          .import_manifest(session_topology,
                           groups,
                           candidate_hash,
                           3,
                           ManifestSummary{
                               .claimed_parent_hash = fromNumber(0),
                               .claimed_group_index = GroupIndex(0),
                               .statement_knowledge = create_filter(
                                   {{false, false, false}, {true, true, true}}),
                           },
                           ManifestKind::Full,
                           ValidatorIndex(0))
          .error(),
      GridTracker::Error::MALFORMED_HAS_SECONDED);
}

TEST_F(GridTrackerTest, reject_insufficient_below_threshold) {
  GridTracker tracker;
  SessionTopologyView session_topology = {View{
      .sending = {},
      .receiving = {ValidatorIndex(0)},
  }};

  const auto groups = dummy_groups(3);
  const auto candidate_hash = fromNumber(42);
  ASSERT_EQ(groups.get_size_and_backing_threshold(GroupIndex(0)),
            std::make_tuple(size_t(3), size_t(2)));

  // only one vote
  ASSERT_EQ(tracker
                .import_manifest(
                    session_topology,
                    groups,
                    candidate_hash,
                    3,
                    ManifestSummary{
                        .claimed_parent_hash = fromNumber(0),
                        .claimed_group_index = GroupIndex(0),
                        .statement_knowledge = create_filter(
                            {{false, false, true}, {false, false, false}}),
                    },
                    ManifestKind::Full,
                    ValidatorIndex(0))
                .error(),
            GridTracker::Error::INSUFFICIENT);

  // seconding + validating still not enough to reach '2' threshold
  ASSERT_EQ(tracker
                .import_manifest(
                    session_topology,
                    groups,
                    candidate_hash,
                    3,
                    ManifestSummary{
                        .claimed_parent_hash = fromNumber(0),
                        .claimed_group_index = GroupIndex(0),
                        .statement_knowledge = create_filter(
                            {{false, false, true}, {false, false, true}}),
                    },
                    ManifestKind::Full,
                    ValidatorIndex(0))
                .error(),
            GridTracker::Error::INSUFFICIENT);

  // finally good.
  const auto res = tracker.import_manifest(
      session_topology,
      groups,
      candidate_hash,
      3,
      ManifestSummary{
          .claimed_parent_hash = fromNumber(0),
          .claimed_group_index = GroupIndex(0),
          .statement_knowledge =
              create_filter({{false, false, true}, {false, true, false}}),
      },
      ManifestKind::Full,
      ValidatorIndex(0));
  ASSERT_TRUE(res.has_value() && res.value() == false);
}
