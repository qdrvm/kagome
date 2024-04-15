#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#
add_subdirectory(vote_graph)
add_subdirectory(voting_round)

addtest(chain_test
    chain_test.cpp
    )
target_link_libraries(chain_test
    logger_for_tests
    )

addtest(vote_tracker_test
    vote_tracker_test.cpp
    )

addtest(voter_set_test
    voter_set_test.cpp
    )

addtest(vote_weight_test
    vote_weight_test.cpp
    )

addtest(authority_manager_test
    authority_manager_test.cpp
)
target_link_libraries(authority_manager_test
    dummy_error
    logger_for_tests
)
