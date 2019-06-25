#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(extrinsic_submission_service
    error.cpp
    error.hpp
    extrinsic_submission_api.hpp
    extrinsic_submission_proxy.hpp
    extrinsic_submission_proxy.cpp
    extrinsic_submission_service.hpp
    extrinsic_submission_service.cpp
    impl/extrinsic_submission_api_impl.hpp
    impl/extrinsic_submission_api_impl.cpp
    json_transport.hpp
    json_transport.cpp
    network_address.hpp
    )
target_link_libraries(extrinsic_submission_service
    buffer
    RapidJSON::rapidjson
    transaction_pool
    )
