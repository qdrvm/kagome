#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(uvarint_test
    uvarint_test.cpp
    )

target_link_libraries(uvarint_test
    uvarint
    hexutil
    )

addtest(protocol_list_test
    protocol_list_test.cpp
    )

addtest(base58_codec_test
    base58_codec_test.cpp
    )

target_link_libraries(base58_codec_test
    base58_codec
    c_utils
    )

addtest(address_converter_test
    address_converter_test.cpp
    )

target_link_libraries(address_converter_test
    converter_utils
    c_utils
    )


addtest(converter_utils_test
    converter_utils_test.cpp
    )

target_link_libraries(converter_utils_test
    converter_utils
    c_utils
    )

addtest(ip_v4_validator_test
    ip_v4_validator_test.cpp
    )

target_link_libraries(ip_v4_validator_test
    ip_v4_validator
    )
