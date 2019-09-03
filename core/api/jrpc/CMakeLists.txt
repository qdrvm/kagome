#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(api_jrpc_processor
    jrpc_processor.hpp
    jrpc_processor.cpp
    value_converter.hpp
    )
target_link_libraries(api_jrpc_processor
    RapidJSON::rapidjson
    )
