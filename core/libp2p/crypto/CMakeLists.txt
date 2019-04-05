#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(crypto
        common.hpp
        crypto_provider.hpp
        key.hpp
        private_key.hpp
        public_key.hpp
        impl/crypto_provider_impl.hpp
        impl/crypto_provider_impl.cpp
        impl/public_key_impl.hpp
        impl/public_key_impl.cpp
        impl/private_key_impl.hpp impl/private_key_impl.cpp)

target_link_libraries(crypto
        buffer
        keys_proto
        openssl
        )

add_subdirectory(proto)
