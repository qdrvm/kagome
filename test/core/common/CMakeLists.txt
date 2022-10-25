##
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
##

addtest(buffer_test
    buffer_test.cpp
    )
target_link_libraries(buffer_test
    fmt::fmt
    hexutil
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

addtest(mp_utils_test
    mp_utils_test.cpp
    )
target_link_libraries(mp_utils_test
    mp_utils
    blob
    fmt::fmt
    )

addtest(variant_builder_test
    variant_builder_test.cpp
    )
target_link_libraries(variant_builder_test
    Boost::boost
    )

addtest(size_limited_containers_test
    size_limited_containers_test.cpp
    )
target_link_libraries(size_limited_containers_test
    fmt::fmt
    )
