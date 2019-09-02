#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(types)

addtest(scale_rpc_reader_libp2p_test
    scale_rpc_reader_libp2p_test.cpp
    )
target_link_libraries(scale_rpc_reader_libp2p_test
    message_read_writer
    scale
    outcome
    )

addtest(scale_rpc_writer_libp2p_test
    scale_rpc_writer_libp2p_test.cpp
    )
target_link_libraries(scale_rpc_writer_libp2p_test
    message_read_writer
    scale
    outcome
    )
