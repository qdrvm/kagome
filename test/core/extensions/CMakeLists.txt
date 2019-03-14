#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(memory_heap_test memory_heap_test.cpp)
target_link_libraries(memory_heap_test
    extensions
    )
