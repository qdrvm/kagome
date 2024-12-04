/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "parachain/pvf/pvf.hpp"

namespace kagome::parachain {

  class PvfMock : public Pvf {
   public:
    MOCK_METHOD(void,
                pvf,
                (const CandidateReceipt &,
                 const ParachainBlock &,
                 const runtime::PersistedValidationData &,
                 Cb),
                (const, override));

    MOCK_METHOD(void,
                pvfValidate,
                (const PersistedValidationData &,
                 const ParachainBlock &,
                 const CandidateReceipt &,
                 const ParachainRuntime &,
                 runtime::PvfExecTimeoutKind,
                 Cb),
                (const, override));
  };

}  // namespace kagome::parachain
