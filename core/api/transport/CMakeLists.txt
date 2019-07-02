#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(api_transport
    basic_transport.hpp
    network_address.hpp
    listener.hpp
    listener.cpp
    session.hpp
    session.cpp
    session_manager.cpp
    worker_api.hpp
    )
target_link_libraries(api_transport
    Boost::boost
    )
