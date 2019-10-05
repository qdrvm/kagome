#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#
add_subdirectory(vote_graph)

addtest(chain_test
    chain_test.cpp
    )
target_link_libraries(chain_test
    grandpa_chain
    )
