#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(chain_api_test
    chain_api_test.cpp
    )
target_link_libraries(chain_api_test
    api
    blob
    )

addtest(base_request_test
    base_request_test.cpp
    )
target_link_libraries(base_request_test
    blob
    )
