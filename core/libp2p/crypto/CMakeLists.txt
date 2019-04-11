#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(crypto_provider
        aes/aes_provider.hpp
        aes/aes_provider.cpp
        common.hpp
        crypto_provider.hpp
        error.hpp
        error.cpp
        hmac/hmac_provider.hpp
        hmac/hmac_provider.cpp
        impl/crypto_provider_impl.hpp
        impl/crypto_provider_impl.cpp
        key.hpp
        key.cpp
        private_key.hpp
        public_key.hpp
        )

target_link_libraries(crypto_provider
        buffer
        keys_proto
        OpenSSL::Crypto
        )

add_subdirectory(proto)
