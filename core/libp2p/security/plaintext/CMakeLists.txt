#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(security_plaintext
    plaintext.cpp
    plaintext.hpp
    )
target_link_libraries(security_plaintext
    plaintext_connection
    outcome
    )
