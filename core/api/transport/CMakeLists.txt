#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(api_transport
    impl/http_session.hpp
    impl/http_session.cpp
    error.hpp
    error.cpp
    listener.hpp
    session.hpp
    impl/listener_impl.hpp
    impl/listener_impl.cpp
    impl/session_impl.hpp
    impl/session_impl.cpp
    )
target_link_libraries(api_transport
    Boost::boost
    )
