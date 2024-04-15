#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(types)

addtest(block_response_cache_test
    block_response_cache_test.cpp
    )

addtest(rpc_libp2p_test
    rpc_libp2p_test.cpp
    )
target_link_libraries(rpc_libp2p_test
    testutil_read_writer_helper
    logger_for_tests
    )

addtest(state_protocol_observer_test
    state_protocol_observer_test.cpp
    )
target_link_libraries(state_protocol_observer_test
    logger_for_tests
    )

addtest(sync_protocol_observer_test
    sync_protocol_observer_test.cpp
    )
target_link_libraries(sync_protocol_observer_test
    logger_for_tests
    )

addtest(synchronizer_test
    synchronizer_test.cpp
    )
target_link_libraries(synchronizer_test
    logger_for_tests
    )
