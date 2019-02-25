##
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
##

addtest(buffer_test
  buffer_test.cpp
  )
target_link_libraries(buffer_test
  buffer
  )

addtest(hexutil_test
  hexutil_test.cpp
  )
target_link_libraries(hexutil_test
  hexutil
  )
