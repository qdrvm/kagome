#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(keccak
        keccak.c
        keccak.h
        )
target_link_libraries(keccak
        OpenSSL::Crypto
        buffer
        blob
        )
kagome_install(keccak)
