#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(telemetry
    impl/service_impl.cpp
    impl/connection_impl.cpp
    impl/message_pool.cpp
    endpoint.hpp
    )
target_link_libraries(telemetry
    spin_lock
    Boost::boost
    Boost::date_time
    OpenSSL::SSL
    RapidJSON::rapidjson
    p2p::p2p_basic_scheduler
    )
kagome_install(telemetry)
