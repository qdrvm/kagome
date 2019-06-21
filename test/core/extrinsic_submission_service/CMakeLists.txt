#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(extrinsics_submission_service_test
    extrinsic_submission_service_test.cpp

    )
target_link_libraries(extrinsics_submission_service_test
    extrinsics_submission_service
    )

addtest(extrinsic_submission_proxy_test
    extrinsic_submission_proxy_test.cpp
    )
target_link_libraries(extrinsic_submission_proxy_test
    extrinsics_submission_service
    )

addtest(extrinsic_submission_api_test
    extrinsic_submission_api_test.cpp
    )
target_link_libraries(extrinsic_submission_api_test
    extrinsics_submission_service
    blob
    hasher
    )
