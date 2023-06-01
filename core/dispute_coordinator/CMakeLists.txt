#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(participation)

add_library(dispute_coordinator
    impl/dispute_coordinator_impl.cpp
    impl/rolling_session_window_impl.cpp
    impl/storage_impl.cpp
    impl/chain_scraper_impl.cpp
    impl/scraped_candidates.cpp
    impl/spam_slots_impl.cpp
    impl/candidate_vote_state.cpp
    impl/runtime_info.cpp
    impl/batch.cpp
    impl/batches.cpp
    impl/sending_dispute.cpp
    impl/errors.cpp
    )

target_link_libraries(dispute_coordinator
    network
    )
