#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(telemetry
    impl/service_impl.cpp
    impl/connection_impl.cpp
    impl/message_pool.cpp
    connection.hpp
    endpoint.hpp
    service.hpp
    service.cpp
    )
target_link_libraries(telemetry
    spin_lock
    kagome_uri
    Boost::boost
    Boost::date_time
    OpenSSL::SSL
    RapidJSON::rapidjson
    p2p::p2p_basic_scheduler
    p2p::p2p_multiaddress
    )
kagome_install(telemetry)
