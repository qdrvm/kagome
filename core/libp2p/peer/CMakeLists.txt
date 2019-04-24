#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(inmem_key_repository)

add_library(libp2p_peer_errors
  errors.cpp
  )
target_link_libraries(libp2p_peer_errors
  outcome
  )
