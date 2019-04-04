#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(uvarint
    uvarint.cpp
    uvarint.hpp
    )

target_link_libraries(uvarint
    c_utils
    outcome
    )

add_library(protocol_list
    protocol_list.cpp
    protocol_list.hpp
    )
