#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(ed25519_provider
    ed25519_provider_impl.hpp
    ed25519_provider_impl.cpp
    )
target_link_libraries(ed25519_provider
    ed25519
    )
