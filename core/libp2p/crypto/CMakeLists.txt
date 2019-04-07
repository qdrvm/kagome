#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(crypto_provider
        common.hpp
        crypto_provider.hpp
        error.hpp
        error.cpp
        key.hpp
        impl/crypto_provider_impl.hpp
        impl/crypto_provider_impl.cpp
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
