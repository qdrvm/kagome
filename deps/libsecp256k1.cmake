#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(libsecp256k1
    libsecp256k1/src/gen_context.c
    libsecp256k1/src/secp256k1.c
    )
target_include_directories(libsecp256k1 PUBLIC
    libsecp256k1/include
    )
disable_clang_tidy(libsecp256k1)
