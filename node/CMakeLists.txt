#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(kagome_block_producing)
add_subdirectory(kagome_validating)
add_subdirectory(kagome_full_syncing)
