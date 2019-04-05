#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

set(SCHEMA_PATH ${CMAKE_CURRENT_SOURCE_DIR})

add_proto_library(keys_proto
        key_type.proto
        private_key.proto
        public_key.proto
        )
