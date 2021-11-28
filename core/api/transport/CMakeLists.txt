#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(api_transport
    impl/http/http_session.hpp
    impl/http/http_session.cpp
    impl/ws/ws_session.hpp
    impl/ws/ws_session.cpp
    error.hpp
    error.cpp
    listener.hpp
    session.hpp
    impl/http/http_listener_impl.hpp
    impl/http/http_listener_impl.cpp
    impl/ws/ws_listener_impl.hpp
    impl/ws/ws_listener_impl.cpp
    )
target_link_libraries(api_transport
    Boost::boost
    logger
    metrics
    )

add_library(rpc_thread_pool
    rpc_io_context.hpp
    rpc_thread_pool.hpp
    rpc_thread_pool.cpp
    )
target_link_libraries(rpc_thread_pool
    Boost::boost
    logger
    )
