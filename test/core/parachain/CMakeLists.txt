#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(availability)

addtest(candidate_storage_test
    candidate_storage.cpp
    )

target_link_libraries(candidate_storage_test
    prospective_parachains
    log_configurator
    base_fs_test
    key_store
    logger
    )

addtest(scope_test
    scope.cpp
    )

target_link_libraries(scope_test
    prospective_parachains
    log_configurator
    base_fs_test
    key_store
    logger
    )

addtest(backing_test
    backing.cpp
    )

target_link_libraries(backing_test
    Boost::Boost.DI
    Boost::boost
    validator_parachain
    prospective_parachains
    log_configurator
    base_fs_test
    key_store
    logger
    )

addtest(candidates_test
    candidates.cpp
    )

target_link_libraries(candidates_test
    prospective_parachains
    validator_parachain
    log_configurator
    base_fs_test
    key_store
    logger
    )

addtest(fragment_chain_test
    fragment_chain.cpp
    )

target_link_libraries(fragment_chain_test
    prospective_parachains
    log_configurator
    base_fs_test
    key_store
    logger
    )

addtest(prospective_parachains_test
    prospective_parachains.cpp
    )

target_link_libraries(prospective_parachains_test
    validator_parachain
    prospective_parachains
    log_configurator
    base_fs_test
    key_store
    logger
    )

addtest(approval_distribution_test
    approval_distribution_test.cpp
    )

target_link_libraries(approval_distribution_test
    validator_parachain
    prospective_parachains
    log_configurator
    vrf_provider
    base_fs_test
    key_store
    logger
    )

addtest(parachain_test
    pvf_test.cpp
    assignments.cpp
    cluster_test.cpp
    grid.cpp
    grid_tracker.cpp
    )

target_link_libraries(parachain_test
    validator_parachain
    log_configurator
    vrf_provider
    base_fs_test
    key_store
    logger
    )

if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_sources(parachain_test PRIVATE secure_mode.cpp)
endif()
