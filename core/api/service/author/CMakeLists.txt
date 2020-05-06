#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(requests)

add_library(author_api_service
    author_api.hpp
    author_jrpc_processor.hpp
    author_jrpc_processor.cpp
    impl/author_api_impl.hpp
    impl/author_api_impl.cpp
)

target_link_libraries(author_api_service
    scale
    logger
    )

add_library(author_api_observer
    author_api_observer.hpp
    impl/author_api_observer_impl.hpp
    impl/author_api_observer_impl.cpp
    )
target_link_libraries(author_api_observer
    logger
    )
