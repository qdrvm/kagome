#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(state_metrics
    state_metrics.hpp
    impl/state_metrics_impl.hpp
    impl/state_metrics_impl.cpp
    )

target_link_libraries(state_metrics
    p2p::p2p_basic_scheduler
    api
    metrics
    logger
    )

kagome_install(state_metrics)
