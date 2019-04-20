#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(aes)
add_subdirectory(hmac)
add_subdirectory(marshaller)
add_subdirectory(proto)
add_subdirectory(random)

add_library(crypto_key
        key.hpp
        key.cpp
        )
target_link_libraries(crypto_key
        buffer
        )

add_library(crypto_provider
        common.hpp
        crypto_provider.hpp
        impl/crypto_provider_impl.hpp
        impl/crypto_provider_impl.cpp
        key.hpp
        private_key.hpp
        public_key.hpp
        )

target_link_libraries(crypto_provider
        aes_provider
        buffer
        crypto_error
        crypto_key
        hmac_provider
        marshaller
        Boost::boost
        )

add_library(crypto_error
        error.hpp
        error.cpp
        )

target_link_libraries(crypto_error
        outcome
        )
