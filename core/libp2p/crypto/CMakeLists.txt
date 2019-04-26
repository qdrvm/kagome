#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(aes)
add_subdirectory(hmac)
add_subdirectory(marshaller)
add_subdirectory(proto)
add_subdirectory(random)

add_library(crypto_error
    error.hpp
    error.cpp
    )

target_link_libraries(crypto_error
    outcome
    )


add_library(crypto_key
    key.cpp
    key.hpp
    )
target_link_libraries(crypto_key
    Boost::boost
    )

add_library(key_generator
    key_generator.hpp
    key_generator/key_generator_impl.cpp
    key_generator/key_generator_impl.hpp)
target_link_libraries(key_generator
    crypto_error
    outcome
    buffer
    random_provider
    OpenSSL::Crypto
    Boost::filesystem
    )
