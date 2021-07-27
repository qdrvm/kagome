/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_RUNTIME_ENVIRONMENT_FACTORY_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_RUNTIME_RUNTIME_ENVIRONMENT_FACTORY_MOCK_HPP

#include "runtime/binaryen/runtime_environment.hpp"
#include "runtime/binaryen/runtime_environment_factory.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime::binaryen {

  class RuntimeEnvironmentFactoryMock : public RuntimeEnvironmentFactory {
   public:
    MOCK_METHOD1(makeIsolated,
                 outcome::result<RuntimeEnvironment>(const Config &));
    MOCK_METHOD0(makePersistent, outcome::result<RuntimeEnvironment>());
    MOCK_METHOD0(makeEphemeral, outcome::result<RuntimeEnvironment>());

    MOCK_METHOD2(makeIsolatedAt,
                 outcome::result<RuntimeEnvironment>(
                     const storage::trie::RootHash &state_root,
                     const Config &));
    MOCK_METHOD1(makePersistentAt,
                 outcome::result<RuntimeEnvironment>(
                     const storage::trie::RootHash &state_root));
    MOCK_METHOD1(makeEphemeralAt,
                 outcome::result<RuntimeEnvironment>(
                     const storage::trie::RootHash &state_root));

    MOCK_METHOD0(reset, void());
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_RUNTIME_ENVIRONMENT_FACTORY_MOCK_HPP
