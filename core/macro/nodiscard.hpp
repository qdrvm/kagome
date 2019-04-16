/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NODISCARD_HPP
#define KAGOME_NODISCARD_HPP

// TODO(warchant): make conditional definition: check if nodiscard is supported by current compiler
#if 1
#define NODISCARD [[nodiscard]]
#endif

#endif //KAGOME_NODISCARD_HPP
