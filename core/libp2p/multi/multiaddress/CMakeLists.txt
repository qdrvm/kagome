#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(multiaddress
    multiaddress.cpp
    c-utils/base58.c
    c-utils/protocols.c
    c-utils/protoutils.c
    c-utils/varhexutils.c
    c-utils/varint.c
    )
