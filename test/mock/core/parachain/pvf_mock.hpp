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
       void pvf(const CandidateReceipt &receipt,
                     const ParachainBlock &pov,
                     const runtime::PersistedValidationData &pvd,
                     Cb cb) const override {
        cb(call_pvf(receipt, pov, pvd));
    }

    void pvfValidate(const PersistedValidationData &pvd,
                 const ParachainBlock &pb,
                 const CandidateReceipt &r,
                 const ParachainRuntime &pr,
                 runtime::PvfExecTimeoutKind kind,
                 Cb cb) const override {
                  cb(call_pvfValidate(pvd, pb, r, pr, kind));
                 }

    MOCK_METHOD(Result,
                call_pvf,
                (const CandidateReceipt &,
                 const ParachainBlock &,
                 const runtime::PersistedValidationData &),
                (const));

    MOCK_METHOD(Result,
                call_pvfValidate,
                (const PersistedValidationData &,
                 const ParachainBlock &,
                 const CandidateReceipt &,
                 const ParachainRuntime &,
                 runtime::PvfExecTimeoutKind),
                (const));
  };

}  // namespace kagome::parachain
