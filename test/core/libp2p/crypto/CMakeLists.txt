#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(keys_test
        keys_test.cpp
        )

target_link_libraries(keys_test
        buffer
        protobuf::libprotobuf-lite
        crypto_provider
        Boost::boost
        )
