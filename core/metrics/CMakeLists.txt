#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(metrics
    exposer.hpp
    handler.hpp
    impl/exposer_impl.cpp
    impl/exposer_impl.hpp
    impl/prometheus/handler_impl.cpp
    impl/prometheus/handler_impl.hpp
    impl/prometheus/metrics_impl.cpp
    impl/prometheus/metrics_impl.hpp
    impl/prometheus/registry_impl.cpp
    impl/prometheus/registry_impl.hpp
    impl/session_impl.cpp
    impl/session_impl.hpp
    metrics.hpp
    registry.hpp
    session.hpp)

target_link_libraries(metrics
    app_state_manager
    Boost::boost
    prometheus-cpp::core)
