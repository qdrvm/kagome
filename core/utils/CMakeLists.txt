#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_executable(storage_explorer storage_explorer.cpp)
target_link_libraries(storage_explorer application_injector)

add_executable(kagome-db-editor
  kagome_db_editor.cpp
  )
target_link_libraries(kagome-db-editor
  Boost::Boost.DI
  changes_tracker
  leveldb_wrapper
  polkadot_codec
  polkadot_trie_factory
  trie_serializer
  trie_storage
  trie_storage_backend
  )
kagome_install(kagome-db-editor)

add_executable(peering-utility
  grandpa_protocol.cpp
  peering.cpp)
target_link_libraries(peering-utility
  app_state_manager
  Backward::Backward
  Boost::Boost.DI
  ed25519_provider
  p2p::p2p
  p2p::p2p_identify
  p2p::p2p_kademlia
  p2p::p2p_ping
  protocol_error
  scale_message_read_writer
  )
