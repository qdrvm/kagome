# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(inmem_address_repository_test
    inmem_address_repository_test.cpp
    )
target_link_libraries(inmem_address_repository_test
    inmem_address_repository
    )
