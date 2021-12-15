#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(child_state_api_test
    child_state_api_test.cpp
    )
target_link_libraries(child_state_api_test
    child_state_api_service
    blob
    )

addtest(child_state_jrpc_processor_test
    child_state_jrpc_processor_test.cpp
    )
target_link_libraries(child_state_jrpc_processor_test
    child_state_api_service
    api_jrpc_server
    blob
    )
