#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(timeline
    impl/timeline_impl.cpp
    impl/slots_util_impl.cpp
    impl/block_appender_base.cpp
    impl/block_executor_impl.cpp
    impl/block_header_appender_impl.cpp
    impl/block_addition_error.cpp
    impl/consensus_selector_impl.cpp
    impl/timeline_error.cpp
    impl/slot_leadership_error.cpp
    )
target_link_libraries(timeline
    logger
    telemetry
    network
    )
kagome_install(timeline)
kagome_clear_objects(timeline)
