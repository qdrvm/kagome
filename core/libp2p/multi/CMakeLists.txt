#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(utils)

add_subdirectory(converters)

add_subdirectory(multibase_codec)

add_library(multihash
  multihash.hpp
  multihash.cpp
  )

target_link_libraries(multihash
  buffer
  hexutil
  uvarint
  outcome
  )

add_library(multistream
  multistream.hpp
  multistream.cpp
  )

target_link_libraries(multistream
  buffer
  uvarint
  outcome
  )


add_library(multiaddress
  multiaddress.cpp
  )
target_link_libraries(multiaddress
  converter_utils
  outcome
  buffer
  )
