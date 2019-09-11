#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(sr25519_provider
    sr25519_provider_impl.hpp
    sr25519_provider_impl.cpp
    )
target_link_libraries(sr25519_provider
    sr25519
    )
