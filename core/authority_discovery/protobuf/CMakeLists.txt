#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_proto_library(authority_discovery_proto
        authority_discovery.v2.proto
        )
target_include_directories(authority_discovery_proto PUBLIC
        # required for common targets
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/pb/authority_discovery_proto>
        # required for compiling proto targets
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/pb/authority_discovery_proto/generated>
        )

add_proto_library(keys_proto
        keys.proto
        )
target_include_directories(keys_proto PUBLIC
        # required for common targets
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/pb/keys_proto>
        # required for compiling proto targets
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/pb/keys_proto/generated>
        )

