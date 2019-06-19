#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(extrinsics_submission_service
    error.cpp
    error.hpp
    extrinsic_submission_api.hpp
    extrinsic_submission_api.cpp
    extrinsic_submission_proxy.hpp
    extrinsic_submission_proxy.cpp
    service.hpp
    service.cpp
    json_transport.hpp
    impl/json_transport_impl.cpp
    impl/json_transport_impl.hpp
    service_configuration.hpp
    impl/service_builder.hpp
    impl/service_builder.cpp
    )
target_link_libraries(extrinsics_submission_service
    buffer
    RapidJSON::rapidjson
    )
