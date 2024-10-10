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
    Boost::process
    OpenSSL::SSL
    RapidJSON::rapidjson
    p2p::p2p_asio_scheduler_backend
    p2p::p2p_multiaddress
    p2p::p2p_peer_id
    )
kagome_install(telemetry)
