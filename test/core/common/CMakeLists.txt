#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(buffer_test
    buffer_test.cpp
    )

addtest(buffer_view_test
    buffer_view_test.cpp
    )

addtest(hexutil_test
    hexutil_test.cpp
    )

addtest(blob_test
    blob_test.cpp
    )

addtest(mp_utils_test
    mp_utils_test.cpp
    )

addtest(variant_builder_test
    variant_builder_test.cpp
    )

addtest(size_limited_containers_test
    size_limited_containers_test.cpp
    )

addtest(small_lru_cache_test
    lru_cache_test.cpp
    )
