#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
#
# Kagome is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Kagome is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

add_library(application_injector
    application_injector.cpp
    )
target_link_libraries(application_injector
    p2p::p2p
    tsl::tsl_hat_trie
    Boost::Boost.DI
    api_transport
    api_jrpc_server
    state_api_service
    readonly_trie_builder
    babe
    babe_lottery
    block_header_repository
    block_storage
    babe_synchronizer
    block_tree
    block_validator
    buffer
    clock
    sr25519_provider
    configuration_storage
    binaryen_core_api
    binaryen_babe_api
    binaryen_grandpa_api
    binaryen_block_builder_api
    extrinsic_api_service
    extension_factory
    epoch_storage
    gossiper_broadcast
    kagome_router
    leveldb
    local_key_storage
    outcome
    launcher
    environment
    proposer
    storage_wasm_provider
    synchronizer
    binaryen_tagged_transaction_queue_api
    vrf_provider
    waitable_timer
    binaryen_wasm_executor
    )
