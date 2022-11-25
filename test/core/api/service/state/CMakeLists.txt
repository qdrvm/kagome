#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(state_api_test
    state_api_test.cpp
    )
target_link_libraries(state_api_test
    api
    blob
    )

addtest(state_jrpc_processor_test
    state_jrpc_processor_test.cpp
    )
target_link_libraries(state_jrpc_processor_test
    api
    blob
    )
