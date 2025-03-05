/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <bitset>
#include <type_traits>
#include "pvm/types.hpp"

namespace kagome::pvm {

    template<size_t N>
    struct BitMask final {
        struct BitIndex {
            uint32_t index;
            uint32_t primary;
            uint32_t secondary;
        };

        using InternalT = std::bitset<N>;
        using Index = BitIndex<N>;

        static BitMask empty_default();

        Index index(uint32_t ix);

    // { t.index(ix) } -> std::convertible_to<typename T::Index>;
    // { t.find_first(idx) } -> std::same_as<Opt<typename T::Index>>;

    // { t.set(idx) };
    // { t.unset(idx) };


    private:
        InternalT data_;
    };

    template<size_t N>
    BitMask<N> BitMask<N>::empty_default() {
        return {};
    }

    template<size_t N>
    BitMask<N>::Index BitMask<N>::index(uint32_t ix) {

    }
}
