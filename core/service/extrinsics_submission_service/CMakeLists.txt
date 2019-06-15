#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(extrinsics_submission_service
    service.hpp
    service.cpp
    )
target_link_libraries(extrinsics_submission_service RapidJSON::rapidjson)
