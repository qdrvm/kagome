#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(api_author_requests INTERFACE)

target_link_libraries(api_author_requests INTERFACE Boost::boost)
