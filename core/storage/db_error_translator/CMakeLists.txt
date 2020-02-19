# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(db_error_translator
    db_error_translator.cpp
    )
target_link_libraries(db_error_translator
    Boost::boost
    in_memory_error
    leveldb_error
    outcome
    )
kagome_install(db_error_translator)
