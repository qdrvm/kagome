#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(hmac_provider
    hmac_provider_impl.hpp
    hmac_provider_impl.cpp
    )

target_link_libraries(hmac_provider
    buffer
    crypto_error
    OpenSSL::Crypto
    )
