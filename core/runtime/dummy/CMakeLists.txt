#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(grandpa_api_dummy
    grandpa_api_dummy.cpp
    )
target_link_libraries(grandpa_api_dummy
    primitives
    )
