#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(api_transport
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
