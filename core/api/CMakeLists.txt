#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(api
    service/impl/api_service_impl.cpp
    service/chain/impl/chain_api_impl.cpp
    service/chain/requests/get_block_hash.cpp
    service/chain/chain_jrpc_processor.cpp
    service/child_state/child_state_jrpc_processor.cpp
    service/child_state/impl/child_state_api_impl.cpp
    service/child_state/requests/get_storage_hash.cpp
    service/child_state/requests/get_storage_size.cpp
    service/child_state/requests/get_keys.cpp
    service/child_state/requests/get_keys_paged.cpp
    service/child_state/requests/get_storage.cpp
    service/rpc/impl/rpc_api_impl.cpp
    service/rpc/requests/methods.cpp
    service/rpc/rpc_jrpc_processor.cpp
    service/state/impl/state_api_impl.cpp
    service/state/requests/call.cpp
    service/state/requests/get_runtime_version.cpp
    service/state/requests/unsubscribe_storage.cpp
    service/state/requests/get_keys_paged.cpp
    service/state/requests/get_storage.cpp
    service/state/requests/subscribe_storage.cpp
    service/state/state_jrpc_processor.cpp
    service/internal/impl/internal_api_impl.cpp
    service/internal/internal_jrpc_processor.cpp
    service/author/impl/author_api_impl.cpp
    service/author/author_jrpc_processor.cpp
    service/system/impl/system_api_impl.cpp
    service/system/requests/health.cpp
    service/system/requests/version.cpp
    service/system/requests/properties.cpp
    service/system/requests/chain.cpp
    service/system/requests/chain_type.cpp
    service/system/requests/name.cpp
    service/system/system_jrpc_processor.cpp
    service/payment/impl/payment_api_impl.cpp
    service/payment/payment_jrpc_processor.cpp
    transport/impl/http/http_listener_impl.cpp
    transport/impl/http/http_session.cpp
    transport/impl/ws/ws_session.cpp
    transport/impl/ws/ws_listener_impl.cpp
    transport/tuner.cpp
    transport/rpc_thread_pool.cpp
    transport/error.cpp
    jrpc/jrpc_handle_batch.cpp
    jrpc/jrpc_server_impl.cpp
    )

target_link_libraries(api
    Boost::boost
    RapidJSON::rapidjson
    metrics
    logger
    app_state_manager
    p2p::p2p_peer_id
    crypto_store
    hexutil
    scale::scale
    storage
    blob
    metadata_api
    babe
    ss58_codec
    outcome
    build_version
    )
kagome_install(api)

add_custom_command(TARGET api
    POST_BUILD
    COMMAND find ${CMAKE_CURRENT_BINARY_DIR} -name "*.o" -type f -delete
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Removing object files from '${CMAKE_CURRENT_BINARY_DIR}'"
    )
