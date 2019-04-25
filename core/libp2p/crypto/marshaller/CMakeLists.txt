#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(marshaller
    key_marshaller.hpp
    key_marshaller.cpp
    )
target_link_libraries(marshaller
    buffer
    crypto_error
    crypto_key
    keys_proto
    )
