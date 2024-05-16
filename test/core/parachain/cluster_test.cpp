#include "parachain/backing/cluster.hpp"

#include <gtest/gtest.h>

#include "testutil/literals.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::network::CandidateHash;
using kagome::network::CompactStatement;
using kagome::network::CompactStatementSeconded;
using kagome::network::CompactStatementValid;
using kagome::parachain::Accept;
using kagome::parachain::ClusterTracker;
using kagome::parachain::RejectIncoming;
using kagome::parachain::RejectOutgoing;
using kagome::parachain::ValidatorIndex;

using StatementVector =
    std::vector<std::pair<ValidatorIndex, CompactStatement>>;

class ClusterTrackerTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {}

  std::vector<ValidatorIndex> group{5, 200, 24, 146};
  size_t seconding_limit = 2;
  ClusterTracker tracker{group, seconding_limit};

  const CandidateHash hash_a =
      "0101010101010101010101010101010101010101010101010101010101010101"_hash256;
  const CandidateHash hash_b =
      "0202020202020202020202020202020202020202020202020202020202020202"_hash256;
  const CandidateHash hash_c =
      "0303030303030303030303030303030303030303030303030303030303030303"_hash256;
};

TEST_F(ClusterTrackerTest, rejects_incoming_outside_of_group) {
  ASSERT_EQ(tracker.can_receive(
                100, 5, CompactStatementSeconded{CandidateHash{hash_a}}),
            outcome::failure(RejectIncoming::NotInGroup));
  ASSERT_EQ(tracker.can_receive(
                5, 100, CompactStatementSeconded{CandidateHash{hash_a}}),
            outcome::failure(RejectIncoming::NotInGroup));
}

TEST_F(ClusterTrackerTest,
       begrudgingly_accepts_too_many_seconded_from_multiple_peers) {
  ASSERT_EQ(tracker.can_receive(5, 5, CompactStatementSeconded(hash_a)),
            outcome::success(Accept::Ok));
  tracker.note_received(5, 5, CompactStatementSeconded(hash_a));
  ASSERT_EQ(tracker.can_receive(5, 5, CompactStatementSeconded(hash_b)),
            outcome::success(Accept::Ok));
  tracker.note_received(5, 5, CompactStatementSeconded(hash_b));
  ASSERT_EQ(tracker.can_receive(5, 5, CompactStatementSeconded(hash_c)),
            outcome::failure(RejectIncoming::ExcessiveSeconded));
}

TEST_F(ClusterTrackerTest, rejects_too_many_seconded_from_sender) {
  ASSERT_EQ(tracker.can_receive(5, 5, CompactStatementSeconded(hash_a)),
            outcome::success(Accept::Ok));
  tracker.note_received(5, 5, CompactStatementSeconded(hash_a));
  ASSERT_EQ(tracker.can_receive(5, 5, CompactStatementSeconded(hash_b)),
            outcome::success(Accept::Ok));
  tracker.note_received(5, 5, CompactStatementSeconded(hash_b));
  ASSERT_EQ(tracker.can_receive(200, 5, CompactStatementSeconded(hash_c)),
            outcome::success(Accept::WithPrejudice));
}

TEST_F(ClusterTrackerTest, rejects_duplicates) {
  tracker.note_received(5, 5, CompactStatementSeconded(hash_a));

  tracker.note_received(5, 200, CompactStatementValid(hash_a));

  ASSERT_EQ(tracker.can_receive(5, 5, CompactStatementSeconded(hash_a)),
            outcome::failure(RejectIncoming::Duplicate));

  ASSERT_EQ(tracker.can_receive(5, 200, CompactStatementValid(hash_a)),
            outcome::failure(RejectIncoming::Duplicate));
}

TEST_F(ClusterTrackerTest, rejects_incoming_valid_without_seconded) {
  ASSERT_EQ(tracker.can_receive(5, 5, CompactStatementValid(hash_a)),
            outcome::failure(RejectIncoming::CandidateUnknown));
}

TEST_F(ClusterTrackerTest, accepts_incoming_valid_after_receiving_seconded) {
  tracker.note_received(5, 200, CompactStatementSeconded(hash_a));

  ASSERT_EQ(tracker.can_receive(5, 5, CompactStatementValid(hash_a)),
            outcome::success(Accept::Ok));
}

TEST_F(ClusterTrackerTest, accepts_incoming_valid_after_outgoing_seconded) {
  tracker.note_sent(5, 200, CompactStatementSeconded(hash_a));

  ASSERT_EQ(tracker.can_receive(5, 5, CompactStatementValid(hash_a)),
            outcome::success(Accept::Ok));
}

TEST_F(ClusterTrackerTest,
       cannot_send_too_many_seconded_even_to_multiple_peers) {
  tracker.note_sent(200, 5, CompactStatementSeconded(hash_a));

  tracker.note_sent(200, 5, CompactStatementSeconded(hash_b));

  ASSERT_EQ(tracker.can_send(200, 5, CompactStatementSeconded(hash_c)),
            outcome::failure(RejectOutgoing::ExcessiveSeconded));

  ASSERT_EQ(tracker.can_send(24, 5, CompactStatementSeconded(hash_c)),
            outcome::failure(RejectOutgoing::ExcessiveSeconded));
}

TEST_F(ClusterTrackerTest, cannot_send_duplicate) {
  tracker.note_sent(200, 5, CompactStatementSeconded(hash_a));

  ASSERT_EQ(tracker.can_send(200, 5, CompactStatementSeconded(hash_a)),
            outcome::failure(RejectOutgoing::Known));
}

TEST_F(ClusterTrackerTest, cannot_send_what_was_received) {
  tracker.note_received(200, 5, CompactStatementSeconded(hash_a));

  ASSERT_EQ(tracker.can_send(200, 5, CompactStatementSeconded(hash_a)),
            outcome::failure(RejectOutgoing::Known));
}

// Ensure statements received with prejudice don't prevent sending later.
TEST_F(ClusterTrackerTest, can_send_statements_received_with_prejudice) {
  tracker = ClusterTracker{group, 1};

  ASSERT_EQ(tracker.can_receive(200, 5, CompactStatementSeconded(hash_a)),
            outcome::success(Accept::Ok));

  tracker.note_received(200, 5, CompactStatementSeconded(hash_a));

  ASSERT_EQ(tracker.can_receive(24, 5, CompactStatementSeconded(hash_b)),
            outcome::success(Accept::WithPrejudice));

  tracker.note_received(24, 5, CompactStatementSeconded(hash_b));

  ASSERT_EQ(tracker.can_send(24, 5, CompactStatementSeconded(hash_a)),
            outcome::success());
}

// Test that the `pending_statements` are set whenever we receive a fresh
// statement.
//
// Also test that pending statements are sorted, with `Seconded` statements in
// the front.
TEST_F(ClusterTrackerTest,
       pending_statements_set_when_receiving_fresh_statements) {
  tracker = ClusterTracker{group, 1};

  // Receive a 'Seconded' statement for candidate A.
  {
    ASSERT_EQ(tracker.can_receive(200, 5, CompactStatementSeconded(hash_a)),
              outcome::success(Accept::Ok));
    tracker.note_received(200, 5, CompactStatementSeconded(hash_a));
    ASSERT_EQ(tracker.pending_statements_for(5),
              (StatementVector{{5, CompactStatementSeconded(hash_a)}}));
    ASSERT_EQ(tracker.pending_statements_for(200), StatementVector{});
    ASSERT_EQ(tracker.pending_statements_for(24),
              (StatementVector{{5, CompactStatementSeconded(hash_a)}}));
    ASSERT_EQ(tracker.pending_statements_for(146),
              (StatementVector{{5, CompactStatementSeconded(hash_a)}}));
  }

  // Receive a 'Valid' statement for candidate A.
  {
    // First, send a `Seconded` statement for the candidate.
    ASSERT_EQ(tracker.can_send(24, 200, CompactStatementSeconded(hash_a)),
              outcome::success());
    tracker.note_sent(24, 200, CompactStatementSeconded(hash_a));

    // We have to see that the candidate is known by the sender, e.g. we sent
    // them 'Seconded' above.
    ASSERT_EQ(tracker.can_receive(24, 200, CompactStatementValid(hash_a)),
              outcome::success(Accept::Ok));
    tracker.note_received(24, 200, CompactStatementValid(hash_a));

    ASSERT_EQ(tracker.pending_statements_for(5),
              (StatementVector{{5, CompactStatementSeconded(hash_a)},
                               {200, CompactStatementValid(hash_a)}}));
    ASSERT_EQ(tracker.pending_statements_for(200),
              (StatementVector{{200, CompactStatementValid(hash_a)}}));
    ASSERT_EQ(tracker.pending_statements_for(24),
              (StatementVector{{5, CompactStatementSeconded(hash_a)}}));
    ASSERT_EQ(tracker.pending_statements_for(146),
              (StatementVector{{5, CompactStatementSeconded(hash_a)},
                               {200, CompactStatementValid(hash_a)}}));
  }

  // Receive a 'Seconded' statement for candidate B.
  {
    ASSERT_EQ(tracker.can_receive(5, 146, CompactStatementSeconded(hash_b)),
              outcome::success(Accept::Ok));
    tracker.note_received(5, 146, CompactStatementSeconded(hash_b));

    ASSERT_EQ(tracker.pending_statements_for(5),
              (StatementVector{{5, CompactStatementSeconded(hash_a)},
                               {200, CompactStatementValid(hash_a)}}));
    ASSERT_EQ(tracker.pending_statements_for(200),
              (StatementVector{
                  {146, CompactStatementSeconded(hash_b)},
                  {200, CompactStatementValid(hash_a)},
              }));
    {
      auto pending_statements = tracker.pending_statements_for(24);
      std::ranges::sort(pending_statements);
      ASSERT_EQ(pending_statements,
                (StatementVector{{5, CompactStatementSeconded(hash_a)},
                                 {146, CompactStatementSeconded(hash_b)}}));
    }
    {
      auto pending_statements = tracker.pending_statements_for(146);
      std::ranges::sort(pending_statements);
      ASSERT_EQ(pending_statements,
                (StatementVector{
                    {5, CompactStatementSeconded(hash_a)},
                    {146, CompactStatementSeconded(hash_b)},
                    {200, CompactStatementValid(hash_a)},
                }));
    }
  }
}

// Test that the `pending_statements` are updated when we send or receive
// statements from others in the cluster.
TEST_F(ClusterTrackerTest, pending_statements_updated_when_sending_statements) {
  tracker = ClusterTracker{group, 1};

  // Receive a 'Seconded' statement for candidate A.
  {
    ASSERT_EQ(tracker.can_receive(200, 5, CompactStatementSeconded(hash_a)),
              outcome::success(Accept::Ok));
    tracker.note_received(200, 5, CompactStatementSeconded(hash_a));

    // Pending statements should be updated.
    ASSERT_EQ(tracker.pending_statements_for(5),
              (StatementVector{{5, CompactStatementSeconded(hash_a)}}));
    ASSERT_EQ(tracker.pending_statements_for(200), StatementVector{});
    ASSERT_EQ(tracker.pending_statements_for(24),
              (StatementVector{{5, CompactStatementSeconded(hash_a)}}));
    ASSERT_EQ(tracker.pending_statements_for(146),
              (StatementVector{{5, CompactStatementSeconded(hash_a)}}));
  }

  // Receive a 'Valid' statement for candidate B.
  {
    // First, send a `Seconded` statement for the candidate.
    ASSERT_EQ(tracker.can_send(24, 200, CompactStatementSeconded(hash_b)),
              outcome::success());
    tracker.note_sent(24, 200, CompactStatementSeconded(hash_b));

    // We have to see the candidate is known by the sender, e.g. we sent them
    // 'Seconded'.
    ASSERT_EQ(tracker.can_receive(24, 200, CompactStatementValid(hash_b)),
              outcome::success(Accept::Ok));
    tracker.note_received(24, 200, CompactStatementValid(hash_b));

    // Pending statements should be updated.
    ASSERT_EQ(tracker.pending_statements_for(5),
              (StatementVector{{5, CompactStatementSeconded(hash_a)},
                               {200, CompactStatementValid(hash_b)}}));
    ASSERT_EQ(tracker.pending_statements_for(200),
              (StatementVector{{200, CompactStatementValid(hash_b)}}));
    ASSERT_EQ(tracker.pending_statements_for(24),
              (StatementVector{{5, CompactStatementSeconded(hash_a)}}));
    ASSERT_EQ(tracker.pending_statements_for(146),
              (StatementVector{{5, CompactStatementSeconded(hash_a)},
                               {200, CompactStatementValid(hash_b)}}));
  }

  // Send a 'Seconded' statement.
  {
    ASSERT_EQ(tracker.can_send(5, 5, CompactStatementSeconded(hash_a)),
              outcome::success());
    tracker.note_sent(5, 5, CompactStatementSeconded(hash_a));

    // Pending statements should be updated.
    ASSERT_EQ(tracker.pending_statements_for(5),
              (StatementVector{{200, CompactStatementValid(hash_b)}}));
    ASSERT_EQ(tracker.pending_statements_for(200),
              (StatementVector{{200, CompactStatementValid(hash_b)}}));
    ASSERT_EQ(tracker.pending_statements_for(24),
              (StatementVector{{5, CompactStatementSeconded(hash_a)}}));
    ASSERT_EQ(tracker.pending_statements_for(146),
              (StatementVector{{5, CompactStatementSeconded(hash_a)},
                               {200, CompactStatementValid(hash_b)}}));
  }

  // Send a 'Valid' statement.
  {
    // First, send a `Seconded` statement for the candidate.
    ASSERT_EQ(tracker.can_send(5, 200, CompactStatementSeconded(hash_b)),
              outcome::success());
    tracker.note_sent(5, 200, CompactStatementSeconded(hash_b));

    // We have to see that the candidate is known by the sender, e.g. we sent
    // them 'Seconded' above.
    ASSERT_EQ(tracker.can_send(5, 200, CompactStatementValid(hash_b)),
              outcome::success());
    tracker.note_sent(5, 200, CompactStatementValid(hash_b));

    // Pending statements should be updated.
    ASSERT_EQ(tracker.pending_statements_for(5), StatementVector{});
    ASSERT_EQ(tracker.pending_statements_for(200),
              (StatementVector{{200, CompactStatementValid(hash_b)}}));
    ASSERT_EQ(tracker.pending_statements_for(24),
              (StatementVector{{5, CompactStatementSeconded(hash_a)}}));
    ASSERT_EQ(tracker.pending_statements_for(146),
              (StatementVector{{5, CompactStatementSeconded(hash_a)},
                               {200, CompactStatementValid(hash_b)}}));
  }
}