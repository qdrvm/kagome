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
    transport.hpp)
target_link_libraries(extrinsics_submission_service
    buffer
    RapidJSON::rapidjson
    )
