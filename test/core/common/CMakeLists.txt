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

addtest(blob_test
  blob_test.cpp
  )
target_link_libraries(blob_test
  blob
  )

addtest(scale_unspecified_test
  scale_unspecified_test.cpp
  )
target_link_libraries(scale_unspecified_test
  scale
  buffer
  )

addtest(scale_compact_test
  scale_compact_test.cpp
  )
target_link_libraries(scale_compact_test
  scale
  buffer
  )

addtest(scale_fixed_test
  scale_fixed_test.cpp
  )
target_link_libraries(scale_fixed_test
  scale
  buffer
  )

addtest(scale_boolean_test
  scale_boolean_test.cpp
  )
target_link_libraries(scale_boolean_test
  scale
  buffer
  )

addtest(scale_optional_test
  scale_optional_test.cpp
  )
target_link_libraries(scale_optional_test
  scale
  buffer
  )

addtest(scale_collection_test
  scale_collection_test.cpp
  )
target_link_libraries(scale_collection_test
  scale
  buffer
  )

addtest(result_test
    result_test.cpp
    )
target_link_libraries(result_test
    Boost::boost
    )
