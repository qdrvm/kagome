#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
#
# Kagome is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Kagome is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

addtest(extrinsic_service_test
    extrinsic_service_test.cpp
    )
target_link_libraries(extrinsic_service_test
    extrinsic_api_service
    api_jrpc_server
    api_transport
    blob
    )

addtest(extrinsic_api_test
    extrinsic_api_test.cpp
    )
target_link_libraries(extrinsic_api_test
    extrinsic_api_service
    blob
    hasher
    testutil_primitives_generator
    )

addtest(extrinsic_api_http_integration_test
    extrinsic_api_http_integration_test.cpp
    )
target_link_libraries(extrinsic_api_http_integration_test
    extrinsic_api_service
    api_transport
    api_jrpc_server
    test_http_client
    )
