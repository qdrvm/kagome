#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(basic_transport
    basic_transport.hpp
    basic_transport.cpp
    )
target_link_libraries(basic_transport
    Boost::boost
)
