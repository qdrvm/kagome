/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define KAGOME_WITH_ASAN 1
#else
#define KAGOME_WITH_ASAN 0
#endif
#endif
