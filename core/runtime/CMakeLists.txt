#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(common)
add_subdirectory(binaryen)
add_subdirectory(wasmedge)
add_subdirectory(wavm)

add_library(executor INTERFACE)
target_link_libraries(executor INTERFACE Boost::boost blob runtime_environment_factory)
kagome_install(executor)

add_subdirectory(runtime_api/impl)
