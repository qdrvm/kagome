#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(key_type_test
    key_type_test.cpp
    )
target_link_libraries(key_type_test
    key_store
    )

addtest(crypto_store_test
    crypto_store_test.cpp
    )
target_link_libraries(crypto_store_test
    key_store
    base_fs_test
    )

addtest(session_keys_test
    session_keys_test.cpp
    )

target_link_libraries(session_keys_test
    key_store
    log_configurator
    )
