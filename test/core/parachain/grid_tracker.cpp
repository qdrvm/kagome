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

TEST_F(GridTrackerTest, senders_can_provide_manifests_in_acknowledgement) {
  const ValidatorIndex validator_index(0);
  GridTracker tracker;
  SessionTopologyView session_topology = {View{
      .sending = {validator_index},
      .receiving = {ValidatorIndex(1)},
  }};

  const auto candidate_hash = fromNumber(42);
  const GroupIndex group_index(0);
  const size_t group_size(3);

  const StatementFilter local_knowledge(group_size);
  const auto groups = dummy_groups(group_size);

  // Add the candidate as backed.
  const auto receivers = tracker.add_backed_candidate(
      session_topology, candidate_hash, group_index, local_knowledge);
  // Validator 0 is in the sending group. Advertise onward to it.
  //
  // Validator 1 is in the receiving group, but we have not received from it, so
  // we're not expected to send it an acknowledgement.
  ASSERT_EQ(receivers.size(), 1);
  ASSERT_EQ(receivers[0].first, validator_index);
  ASSERT_EQ(receivers[0].second, ManifestKind::Full);

  // Note the manifest as 'sent' to validator 0.
  tracker.manifest_sent_to(
      groups, validator_index, candidate_hash, local_knowledge);

  // Import manifest of kind `Acknowledgement` from validator 0.
  const auto ack = tracker.import_manifest(
      session_topology,
      groups,
      candidate_hash,
      3,
      ManifestSummary{
          .claimed_parent_hash = fromNumber(0),
          .claimed_group_index = group_index,
          .statement_knowledge =
              create_filter({{false, true, false}, {true, false, true}}),
      },
      ManifestKind::Acknowledgement,
      validator_index);
  ASSERT_TRUE(ack.has_value() && ack.value() == false);
}

TEST_F(GridTrackerTest,
       pending_communication_receiving_manifest_on_confirmed_candidate) {
  const ValidatorIndex validator_index(0);
  GridTracker tracker;
  SessionTopologyView session_topology = {View{
      .sending = {validator_index},
      .receiving = {ValidatorIndex(1)},
  }};

  const auto candidate_hash = fromNumber(42);
  const GroupIndex group_index(0);
  const size_t group_size(3);

  const StatementFilter local_knowledge(group_size);
  const auto groups = dummy_groups(group_size);

  // Manifest should not be pending yet.
  ASSERT_FALSE(
      tracker.is_manifest_pending_for(validator_index, candidate_hash));

  // Add the candidate as backed.
  tracker.add_backed_candidate(
      session_topology, candidate_hash, group_index, local_knowledge);

  // Manifest should be pending as `Full`.
  ASSERT_EQ(tracker.is_manifest_pending_for(validator_index, candidate_hash),
            ManifestKind::Full);

  // Note the manifest as 'sent' to validator 0.
  tracker.manifest_sent_to(
      groups, validator_index, candidate_hash, local_knowledge);

  // Import manifest.
  //
  // Should overwrite existing `Full` manifest.
  const auto ack = tracker.import_manifest(
      session_topology,
      groups,
      candidate_hash,
      3,
      ManifestSummary{
          .claimed_parent_hash = fromNumber(0),
          .claimed_group_index = group_index,
          .statement_knowledge =
              create_filter({{false, true, false}, {true, false, true}}),
      },
      ManifestKind::Acknowledgement,
      validator_index);
  ASSERT_TRUE(ack.has_value() && ack.value() == false);
  ASSERT_FALSE(
      tracker.is_manifest_pending_for(validator_index, candidate_hash));
}

TEST_F(GridTrackerTest, pending_communication_is_cleared) {
  const ValidatorIndex validator_index(0);
  GridTracker tracker;
  SessionTopologyView session_topology = {View{
      .sending = {},
      .receiving = {validator_index},
  }};

  const auto candidate_hash = fromNumber(42);
  const GroupIndex group_index(0);
  const size_t group_size(3);

  const StatementFilter local_knowledge(group_size);
  const auto groups = dummy_groups(group_size);

  // Add the candidate as backed.
  tracker.add_backed_candidate(
      session_topology, candidate_hash, group_index, local_knowledge);

  // Manifest should not be pending yet.
  ASSERT_FALSE(
      tracker.is_manifest_pending_for(validator_index, candidate_hash));

  // Import manifest. The candidate is confirmed backed and we are expected to
  // receive from validator 0, so send it an acknowledgement.
  const auto ack = tracker.import_manifest(
      session_topology,
      groups,
      candidate_hash,
      3,
      ManifestSummary{
          .claimed_parent_hash = fromNumber(0),
          .claimed_group_index = group_index,
          .statement_knowledge =
              create_filter({{false, true, false}, {true, false, true}}),
      },
      ManifestKind::Full,
      validator_index);
  ASSERT_TRUE(ack.has_value() && ack.value() == true);

  // Acknowledgement manifest should be pending.
  ASSERT_EQ(tracker.is_manifest_pending_for(validator_index, candidate_hash),
            ManifestKind::Acknowledgement);

  // Note the candidate as advertised.
  tracker.manifest_sent_to(
      groups, validator_index, candidate_hash, local_knowledge);

  // Pending manifest should be cleared.
  ASSERT_FALSE(
      tracker.is_manifest_pending_for(validator_index, candidate_hash));
}

TEST_F(GridTrackerTest,
       pending_statements_are_updated_after_manifest_exchange) {
  const ValidatorIndex send_to(0);
  const ValidatorIndex receive_from(1);

  GridTracker tracker;
  SessionTopologyView session_topology = {View{
      .sending = {send_to},
      .receiving = {receive_from},
  }};

  const auto candidate_hash = fromNumber(42);
  const GroupIndex group_index(0);
  const size_t group_size(3);

  const StatementFilter local_knowledge(group_size);
  const auto groups = dummy_groups(group_size);

  // Confirm the candidate.
  const auto receivers = tracker.add_backed_candidate(
      session_topology, candidate_hash, group_index, local_knowledge);
  ASSERT_EQ(receivers.size(), 1);
  ASSERT_EQ(receivers[0].first, send_to);
  ASSERT_EQ(receivers[0].second, ManifestKind::Full);

  // Learn a statement from a different validator.
  tracker.learned_fresh_statement(
      groups,
      session_topology,
      ValidatorIndex(2),
      kagome::network::vstaging::SecondedCandidateHash{
          .hash = candidate_hash,
      });

  // Test receiving followed by sending an ack.
  {
    // Should start with no pending statements.
    ASSERT_FALSE(tracker.pending_statements_for(receive_from, candidate_hash));
    ASSERT_TRUE(tracker.all_pending_statements_for(receive_from).empty());
    const auto ack = tracker.import_manifest(
        session_topology,
        groups,
        candidate_hash,
        3,
        ManifestSummary{
            .claimed_parent_hash = fromNumber(0),
            .claimed_group_index = group_index,
            .statement_knowledge =
                create_filter({{false, true, false}, {true, false, true}}),
        },
        ManifestKind::Full,
        receive_from);
    ASSERT_TRUE(ack.has_value() && ack.value() == true);

    // Send ack now.
    tracker.manifest_sent_to(
        groups, receive_from, candidate_hash, local_knowledge);

    // There should be pending statements now.
    ASSERT_EQ(tracker.pending_statements_for(receive_from, candidate_hash),
              create_filter({{false, false, true}, {false, false, false}}));

    const auto res = tracker.all_pending_statements_for(receive_from);
    ASSERT_EQ(res.size(), 1);
    ASSERT_EQ(
        res[0],
        std::make_pair(ValidatorIndex(2),
                       kagome::network::vstaging::CompactStatement(
                           kagome::network::vstaging::SecondedCandidateHash{
                               .hash = candidate_hash,
                           })));
  }

  // Test sending followed by receiving an ack.
  {
    // Should start with no pending statements.
    ASSERT_FALSE(tracker.pending_statements_for(send_to, candidate_hash));
    ASSERT_TRUE(tracker.all_pending_statements_for(send_to).empty());

    tracker.manifest_sent_to(groups, send_to, candidate_hash, local_knowledge);
    const auto ack = tracker.import_manifest(
        session_topology,
        groups,
        candidate_hash,
        3,
        ManifestSummary{
            .claimed_parent_hash = fromNumber(0),
            .claimed_group_index = group_index,
            .statement_knowledge =
                create_filter({{false, true, false}, {false, false, true}}),
        },
        ManifestKind::Acknowledgement,
        send_to);
    ASSERT_TRUE(ack.has_value() && ack.value() == false);

    // There should be pending statements now.
    ASSERT_EQ(tracker.pending_statements_for(send_to, candidate_hash),
              create_filter({{false, false, true}, {false, false, false}}));
    const auto res = tracker.all_pending_statements_for(send_to);
    ASSERT_EQ(res.size(), 1);
    ASSERT_EQ(
        res[0],
        std::make_pair(ValidatorIndex(2),
                       kagome::network::vstaging::CompactStatement(
                           kagome::network::vstaging::SecondedCandidateHash{
                               .hash = candidate_hash,
                           })));
  }
}

TEST_F(GridTrackerTest, invalid_fresh_statement_import) {
  const ValidatorIndex validator_index(0);

  GridTracker tracker;
  SessionTopologyView session_topology = {View{
      .sending = {},
      .receiving = {validator_index},
  }};

  const auto candidate_hash = fromNumber(42);
  const GroupIndex group_index(0);
  const size_t group_size(3);

  const StatementFilter local_knowledge(group_size);
  const auto groups = dummy_groups(group_size);

  // Should start with no pending statements.
  ASSERT_FALSE(tracker.pending_statements_for(validator_index, candidate_hash));
  ASSERT_TRUE(tracker.all_pending_statements_for(validator_index).empty());

  // Try to import fresh statement. Candidate not backed.
  tracker.learned_fresh_statement(
      groups,
      session_topology,
      validator_index,
      kagome::network::vstaging::SecondedCandidateHash{
          .hash = candidate_hash,
      });

  ASSERT_FALSE(tracker.pending_statements_for(validator_index, candidate_hash));
  ASSERT_TRUE(tracker.all_pending_statements_for(validator_index).empty());

  // Add the candidate as backed.
  tracker.add_backed_candidate(
      session_topology, candidate_hash, group_index, local_knowledge);

  // Try to import fresh statement. Unknown group for validator index.
  tracker.learned_fresh_statement(
      groups,
      session_topology,
      ValidatorIndex(1),
      kagome::network::vstaging::SecondedCandidateHash{
          .hash = candidate_hash,
      });

  ASSERT_FALSE(tracker.pending_statements_for(validator_index, candidate_hash));
  ASSERT_TRUE(tracker.all_pending_statements_for(validator_index).empty());
}

TEST_F(GridTrackerTest,
       pending_statements_updated_when_importing_fresh_statement) {
  const ValidatorIndex validator_index(0);

  GridTracker tracker;
  SessionTopologyView session_topology = {View{
      .sending = {},
      .receiving = {validator_index},
  }};

  const auto candidate_hash = fromNumber(42);
  const GroupIndex group_index(0);
  const size_t group_size(3);

  const StatementFilter local_knowledge(group_size);
  const auto groups = dummy_groups(group_size);

  // Should start with no pending statements.
  ASSERT_FALSE(tracker.pending_statements_for(validator_index, candidate_hash));
  ASSERT_TRUE(tracker.all_pending_statements_for(validator_index).empty());

  // Add the candidate as backed.
  tracker.add_backed_candidate(
      session_topology, candidate_hash, group_index, local_knowledge);

  // Import fresh statement.
  const auto ack = tracker.import_manifest(
      session_topology,
      groups,
      candidate_hash,
      3,
      ManifestSummary{
          .claimed_parent_hash = fromNumber(0),
          .claimed_group_index = group_index,
          .statement_knowledge =
              create_filter({{false, true, false}, {true, false, true}}),
      },
      ManifestKind::Full,
      validator_index);
  ASSERT_TRUE(ack.has_value() && ack.value() == true);

  tracker.manifest_sent_to(
      groups, validator_index, candidate_hash, local_knowledge);
  tracker.learned_fresh_statement(
      groups,
      session_topology,
      validator_index,
      kagome::network::vstaging::SecondedCandidateHash{
          .hash = candidate_hash,
      });

  // There should be pending statements now.
  ASSERT_EQ(tracker.pending_statements_for(validator_index, candidate_hash),
            create_filter({{true, false, false}, {false, false, false}}));

  {
    const auto res = tracker.all_pending_statements_for(validator_index);
    ASSERT_EQ(res.size(), 1);
    ASSERT_EQ(
        res[0],
        std::make_pair(ValidatorIndex(0),
                       kagome::network::vstaging::CompactStatement(
                           kagome::network::vstaging::SecondedCandidateHash{
                               .hash = candidate_hash,
                           })));
  }

  // After successful import, try importing again. Nothing should change.
  tracker.learned_fresh_statement(
      groups,
      session_topology,
      validator_index,
      kagome::network::vstaging::SecondedCandidateHash{
          .hash = candidate_hash,
      });
  ASSERT_EQ(tracker.pending_statements_for(validator_index, candidate_hash),
            create_filter({{true, false, false}, {false, false, false}}));

  {
    const auto res = tracker.all_pending_statements_for(validator_index);
    ASSERT_EQ(res.size(), 1);
    ASSERT_EQ(
        res[0],
        std::make_pair(ValidatorIndex(0),
                       kagome::network::vstaging::CompactStatement(
                           kagome::network::vstaging::SecondedCandidateHash{
                               .hash = candidate_hash,
                           })));
  }
}

TEST_F(GridTrackerTest, pending_statements_respect_remote_knowledge) {
  const ValidatorIndex validator_index(0);

  GridTracker tracker;
  SessionTopologyView session_topology = {View{
      .sending = {},
      .receiving = {validator_index},
  }};

  const auto candidate_hash = fromNumber(42);
  const GroupIndex group_index(0);
  const size_t group_size(3);

  const StatementFilter local_knowledge(group_size);
  const auto groups = dummy_groups(group_size);

  // Should start with no pending statements.
  ASSERT_FALSE(tracker.pending_statements_for(validator_index, candidate_hash));
  ASSERT_TRUE(tracker.all_pending_statements_for(validator_index).empty());

  // Add the candidate as backed.
  tracker.add_backed_candidate(
      session_topology, candidate_hash, group_index, local_knowledge);

  // Import fresh statement.
  const auto ack = tracker.import_manifest(
      session_topology,
      groups,
      candidate_hash,
      3,
      ManifestSummary{
          .claimed_parent_hash = fromNumber(0),
          .claimed_group_index = group_index,
          .statement_knowledge =
              create_filter({{true, false, true}, {false, false, false}}),
      },
      ManifestKind::Full,
      validator_index);
  ASSERT_TRUE(ack.has_value() && ack.value() == true);

  tracker.manifest_sent_to(
      groups, validator_index, candidate_hash, local_knowledge);
  tracker.learned_fresh_statement(
      groups,
      session_topology,
      validator_index,
      kagome::network::vstaging::SecondedCandidateHash{
          .hash = candidate_hash,
      });
  tracker.learned_fresh_statement(groups,
                                  session_topology,
                                  validator_index,
                                  kagome::network::vstaging::ValidCandidateHash{
                                      .hash = candidate_hash,
                                  });

  // The pending statements should respect the remote knowledge (meaning the
  // Seconded statement is ignored, but not the Valid statement).
  ASSERT_EQ(tracker.pending_statements_for(validator_index, candidate_hash),
            create_filter({{false, false, false}, {true, false, false}}));

  {
    const auto res = tracker.all_pending_statements_for(validator_index);
    ASSERT_EQ(res.size(), 1);
    ASSERT_EQ(res[0],
              std::make_pair(ValidatorIndex(0),
                             kagome::network::vstaging::CompactStatement(
                                 kagome::network::vstaging::ValidCandidateHash{
                                     .hash = candidate_hash,
                                 })));
  }
}

TEST_F(GridTrackerTest, pending_statements_cleared_when_sending) {
  const ValidatorIndex validator_index(0);
  const ValidatorIndex counterparty(1);

  GridTracker tracker;
  SessionTopologyView session_topology = {View{
      .sending = {},
      .receiving = {validator_index, counterparty},
  }};

  const auto candidate_hash = fromNumber(42);
  const GroupIndex group_index(0);
  const size_t group_size(3);

  const StatementFilter local_knowledge(group_size);
  const auto groups = dummy_groups(group_size);

  // Should start with no pending statements.
  ASSERT_FALSE(tracker.pending_statements_for(validator_index, candidate_hash));
  ASSERT_TRUE(tracker.all_pending_statements_for(validator_index).empty());

  // Add the candidate as backed.
  tracker.add_backed_candidate(
      session_topology, candidate_hash, group_index, local_knowledge);

  // Import statement for originator.
  ASSERT_TRUE(
      tracker
          .import_manifest(session_topology,
                           groups,
                           candidate_hash,
                           3,
                           ManifestSummary{
                               .claimed_parent_hash = fromNumber(0),
                               .claimed_group_index = group_index,
                               .statement_knowledge = create_filter(
                                   {{false, true, false}, {true, false, true}}),
                           },
                           ManifestKind::Full,
                           validator_index)
          .has_value());

  tracker.manifest_sent_to(
      groups, validator_index, candidate_hash, local_knowledge);
  tracker.learned_fresh_statement(
      groups,
      session_topology,
      validator_index,
      kagome::network::vstaging::SecondedCandidateHash{
          .hash = candidate_hash,
      });

  // Import statement for counterparty.
  ASSERT_TRUE(
      tracker
          .import_manifest(session_topology,
                           groups,
                           candidate_hash,
                           3,
                           ManifestSummary{
                               .claimed_parent_hash = fromNumber(0),
                               .claimed_group_index = group_index,
                               .statement_knowledge = create_filter(
                                   {{false, true, false}, {true, false, true}}),
                           },
                           ManifestKind::Full,
                           counterparty)
          .has_value());

  tracker.manifest_sent_to(
      groups, counterparty, candidate_hash, local_knowledge);
  tracker.learned_fresh_statement(
      groups,
      session_topology,
      counterparty,
      kagome::network::vstaging::SecondedCandidateHash{
          .hash = candidate_hash,
      });

  // There should be pending statements now.
  ASSERT_EQ(tracker.pending_statements_for(validator_index, candidate_hash),
            create_filter({{true, false, false}, {false, false, false}}));
  {
    const auto res = tracker.all_pending_statements_for(validator_index);
    ASSERT_EQ(res.size(), 1);
    ASSERT_EQ(
        res[0],
        std::make_pair(ValidatorIndex(0),
                       kagome::network::vstaging::CompactStatement(
                           kagome::network::vstaging::SecondedCandidateHash{
                               .hash = candidate_hash,
                           })));
  }

  ASSERT_EQ(tracker.pending_statements_for(counterparty, candidate_hash),
            create_filter({{true, false, false}, {false, false, false}}));
  {
    const auto res = tracker.all_pending_statements_for(counterparty);
    ASSERT_EQ(res.size(), 1);
    ASSERT_EQ(
        res[0],
        std::make_pair(ValidatorIndex(0),
                       kagome::network::vstaging::CompactStatement(
                           kagome::network::vstaging::SecondedCandidateHash{
                               .hash = candidate_hash,
                           })));
  }

  tracker.learned_fresh_statement(
      groups,
      session_topology,
      validator_index,
      kagome::network::vstaging::SecondedCandidateHash{
          .hash = candidate_hash,
      });
  tracker.sent_or_received_direct_statement(
      groups,
      validator_index,
      counterparty,
      kagome::network::vstaging::SecondedCandidateHash{
          .hash = candidate_hash,
      },
      false);

  // There should be no pending statements now (for the counterparty).
  ASSERT_EQ(tracker.pending_statements_for(counterparty, candidate_hash),
            StatementFilter(group_size));
  ASSERT_TRUE(tracker.all_pending_statements_for(counterparty).empty());
}

TEST_F(GridTrackerTest, session_grid_topology_consistent) {
  const size_t n_validators = 300;
  const size_t group_size = 5;

  std::vector<std::vector<ValidatorIndex>> groups;
  groups.resize(n_validators / group_size);

  std::vector<ValidatorIndex> validator_indices;
  validator_indices.resize(n_validators);

  for (size_t i = 0; i < n_validators; ++i) {
    validator_indices[i] = ValidatorIndex(i);
    groups[i / group_size].emplace_back(i);
  }

  std::vector<Views> computed_topologies;
  for (size_t i = 0; i < n_validators; ++i) {
    computed_topologies.emplace_back(
        makeViews(groups, validator_indices, ValidatorIndex(i)));
  }

  auto pairwise_check_topologies = [&](auto i, auto j) {
    ValidatorIndex v_i(i);
    ValidatorIndex v_j(j);

    for (GroupIndex group = 0; group < groups.size(); ++group) {
      const auto &g_i = computed_topologies[i][group];
      const auto &g_j = computed_topologies[j][group];

      if (g_i.sending.contains(v_j)) {
        ASSERT_TRUE(g_j.receiving.contains(v_i));
      }

      if (g_j.sending.contains(v_i)) {
        ASSERT_TRUE(g_i.receiving.contains(v_j));
      }

      if (g_i.receiving.contains(v_j)) {
        ASSERT_TRUE(g_j.sending.contains(v_i));
      }

      if (g_j.receiving.contains(v_i)) {
        ASSERT_TRUE(g_i.sending.contains(v_j));
      }
    }
  };

  for (ValidatorIndex i = 0; i < n_validators; ++i) {
    for (ValidatorIndex j = i + 1; j < n_validators; ++j) {
      pairwise_check_topologies(i, j);
    }
  }
}

TEST_F(GridTrackerTest, knowledge_rejects_conflicting_manifest) {
  ReceivedManifests knowledge;

  const ManifestSummary expected_manifest_summary{
      .claimed_parent_hash = fromNumber(2),
      .claimed_group_index = GroupIndex(0),
      .statement_knowledge =
          create_filter({{true, true, false}, {false, true, true}}),
  };

  ASSERT_TRUE(
      knowledge.import_received(3, 2, fromNumber(1), expected_manifest_summary)
          .has_value());

  // conflicting group
  {
    auto s = expected_manifest_summary;
    s.claimed_group_index = GroupIndex(1);
    ASSERT_OUTCOME_ERROR(knowledge.import_received(3, 2, fromNumber(1), s),
                         GridTracker::Error::CONFLICTING);
  }

  // conflicting parent hash
  {
    auto s = expected_manifest_summary;
    s.claimed_parent_hash = fromNumber(3);
    ASSERT_OUTCOME_ERROR(knowledge.import_received(3, 2, fromNumber(1), s),
                         GridTracker::Error::CONFLICTING);
  }

  // conflicting seconded statements bitfield
  {
    auto s = expected_manifest_summary;
    s.statement_knowledge.seconded_in_group.bits = {false, true, false};
    ASSERT_OUTCOME_ERROR(knowledge.import_received(3, 2, fromNumber(1), s),
                         GridTracker::Error::CONFLICTING);
  }

  // conflicting valid statements bitfield
  {
    auto s = expected_manifest_summary;
    s.statement_knowledge.validated_in_group.bits = {false, true, false};
    ASSERT_OUTCOME_ERROR(knowledge.import_received(3, 2, fromNumber(1), s),
                         GridTracker::Error::CONFLICTING);
  }
}

TEST_F(GridTrackerTest, reject_overflowing_manifests) {
  ReceivedManifests knowledge;

  ASSERT_OUTCOME_SUCCESS(knowledge.import_received(
      3,
      2,
      fromNumber(1),
      ManifestSummary{
          .claimed_parent_hash = fromNumber(0xA),
          .claimed_group_index = GroupIndex(0),
          .statement_knowledge =
              create_filter({{true, true, false}, {false, true, true}}),
      }));

  ASSERT_OUTCOME_SUCCESS(knowledge.import_received(
      3,
      2,
      fromNumber(2),
      ManifestSummary{
          .claimed_parent_hash = fromNumber(0xB),
          .claimed_group_index = GroupIndex(0),
          .statement_knowledge =
              create_filter({{true, false, true}, {false, true, true}}),
      }));

  // Reject a seconding validator that is already at the seconding limit.
  // Seconding counts for the validators should not be applied.
  ASSERT_OUTCOME_ERROR(knowledge.import_received(
                           3,
                           2,
                           fromNumber(3),
                           ManifestSummary{
                               .claimed_parent_hash = fromNumber(0xC),
                               .claimed_group_index = GroupIndex(0),
                               .statement_knowledge = create_filter(
                                   {{true, true, true}, {false, true, true}}),
                           }),
                       GridTracker::Error::SECONDING_OVERFLOW);

  // Don't reject validators that have seconded less than the limit so far.
  ASSERT_OUTCOME_SUCCESS(knowledge.import_received(
      3,
      2,
      fromNumber(3),
      ManifestSummary{
          .claimed_parent_hash = fromNumber(0xC),
          .claimed_group_index = GroupIndex(0),
          .statement_knowledge =
              create_filter({{false, true, true}, {false, true, true}}),
      }));
}
