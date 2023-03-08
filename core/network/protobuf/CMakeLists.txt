#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_proto_library(node_api_proto
    api.v1.proto
    )
target_include_directories(node_api_proto PUBLIC
    # required for common targets
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/pb/node_api_proto>
    # required for compiling proto targets
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/pb/node_api_proto/generated>
    )

add_proto_library(light_api_proto
    light.v1.proto
    )
target_include_directories(light_api_proto PUBLIC
    # required for common targets
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/pb/light_api_proto>
    # required for compiling proto targets
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/pb/light_api_proto/generated>
    )
