#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(api_jrpc_server
    jrpc_server_impl.cpp
    value_converter.hpp
    )
target_link_libraries(api_jrpc_server
    RapidJSON::rapidjson
    metrics
    )
