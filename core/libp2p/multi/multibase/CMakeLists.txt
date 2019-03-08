#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(multibase
    multibase_codec_impl.cpp
    codecs/base16.cpp
    codecs/base58.cpp
    codecs/base64.cpp
    )
target_link_libraries(multibase
    buffer
    hexutil
    )
