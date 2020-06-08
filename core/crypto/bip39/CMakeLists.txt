#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(bip39_provider
    impl/bip39_provider_impl.cpp
    bip39_entropy.cpp
    dictionary.cpp
    )
target_link_libraries(bip39_provider
    PUBLIC OpenSSL::SSL
    OpenSSL::Crypto
    ed25519_provider
    sr25519_provider
    buffer
    blob
    )
kagome_install(bip39_provider)
