#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(io_extension_test
    io_extension_test.cpp
    )
target_link_libraries(io_extension_test
    extensions
    )
