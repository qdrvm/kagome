#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(bip39_provider
    dictionary.cpp
    entropy_accumulator.cpp
    impl/bip39_provider_impl.cpp
    mnemonic.cpp
    )
target_link_libraries(bip39_provider
    PUBLIC OpenSSL::SSL
    OpenSSL::Crypto
    buffer
    blob
    logger
    pbkdf2_provider
    sha
    )
kagome_install(bip39_provider)
