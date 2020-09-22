#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(adapter_errors
        adapter_errors.hpp
        adapter_errors.cpp
        )
target_link_libraries(adapter_errors
        outcome
        )
