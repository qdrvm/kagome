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

add_library(binaryen_wasm_memory
    wasm_memory_impl.hpp
    wasm_memory_impl.cpp
    )
target_link_libraries(binaryen_wasm_memory
    buffer
    binaryen::binaryen
    )

add_library(binaryen_runtime_external_interface
    runtime_external_interface.hpp
    runtime_external_interface.cpp
    )
target_link_libraries(binaryen_runtime_external_interface
    binaryen::binaryen
    binaryen_wasm_memory
    logger
    )

add_library(binaryen_wasm_executor
    wasm_executor.hpp
    wasm_executor.cpp
    )
target_link_libraries(binaryen_wasm_executor
    binaryen::binaryen
    logger
    )

add_library(binaryen_runtime_api INTERFACE)
target_link_libraries(binaryen_runtime_api INTERFACE
    binaryen_runtime_external_interface
    )

add_library(binaryen_core_api
    runtime_api/core_impl.cpp
    )
target_link_libraries(binaryen_core_api
    binaryen_wasm_executor
    binaryen_runtime_api
    )

add_library(binaryen_tagged_transaction_queue_api
    runtime_api/tagged_transaction_queue_impl.cpp
    )
target_link_libraries(binaryen_tagged_transaction_queue_api
    binaryen_wasm_executor
    binaryen_runtime_api
    )

add_library(binaryen_block_builder_api
    runtime_api/block_builder_impl.cpp
    )
target_link_libraries(binaryen_block_builder_api
    primitives
    binaryen_wasm_executor
    binaryen_runtime_api
    )

add_library(binaryen_parachain_host_api
    runtime_api/parachain_host_impl.cpp
    )
target_link_libraries(binaryen_parachain_host_api
    binaryen_wasm_executor
    binaryen_runtime_api
    )

add_library(binaryen_grandpa_api
    runtime_api/grandpa_impl.cpp
    )
target_link_libraries(binaryen_grandpa_api
    binaryen_wasm_executor
    binaryen_runtime_api
    )

add_library(binaryen_metadata_api
    runtime_api/metadata_impl.cpp
    )
target_link_libraries(binaryen_metadata_api
    binaryen_wasm_executor
    binaryen_runtime_api
    )

add_library(binaryen_offchain_worker_api
    runtime_api/offchain_worker_impl.cpp
    )
target_link_libraries(binaryen_offchain_worker_api
    binaryen_wasm_executor
    binaryen_runtime_api
    )

add_library(binaryen_babe_api
    runtime_api/babe_api_impl.cpp
    )
target_link_libraries(binaryen_babe_api
    binaryen_wasm_executor
    binaryen_runtime_api
    )
