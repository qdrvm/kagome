#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(extrinsic)

addtest(extrinsic_api_integration_test
    integration_test.cpp
    )
target_link_libraries(extrinsic_api_integration_test
    extrinsic_api_service
    )
