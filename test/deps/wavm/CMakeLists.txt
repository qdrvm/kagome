#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_executable(wavm_try_catch
    try_catch.cpp
    )
target_link_libraries(wavm_try_catch
    WAVM::libWAVM
    )
