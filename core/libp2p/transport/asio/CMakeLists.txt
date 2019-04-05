# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(asio_app
  asio_app.cpp
  )
target_link_libraries(asio_app
  Boost::boost
  Boost::thread
  )
