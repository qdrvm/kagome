#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(multiselect
    multiselect.cpp
    multiselect.hpp
    message_manager.cpp
    message_manager.hpp
    )
target_link_libraries(multiselect
    buffer
    uvarint
    multihash
    )
