#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(extrinsic_api_service
    error.cpp
    error.hpp
    extrinsic_api.hpp
    service.hpp
    service.cpp
    impl/extrinsic_api_impl.hpp
    impl/extrinsic_api_impl.cpp
    request/submit_extrinsic.hpp
    request/submit_extrinsic.cpp
    )
target_link_libraries(extrinsic_api_service
    buffer
    RapidJSON::rapidjson
    transaction_pool
    )
