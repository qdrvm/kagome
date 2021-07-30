##
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
##

addtest(ticker_test
    ticker_test.cpp
    )
target_link_libraries(ticker_test
    ticker
    )
