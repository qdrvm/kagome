#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(router_test
    router_test.cpp
    )
target_link_libraries(router_test
    libp2p_router
    )
