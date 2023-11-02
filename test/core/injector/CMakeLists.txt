#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(application_injector_test application_injector_test.cpp)
target_link_libraries(application_injector_test
    application_injector
    )
