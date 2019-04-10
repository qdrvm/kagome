#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(yamux_test
    yamux_test.cpp
    )
target_link_libraries(yamux_test
    yamux
    )

addtest(yamux_frame_test
    yamux_frame_test.cpp
    )
target_link_libraries(yamux_frame_test
    yamux
    )
