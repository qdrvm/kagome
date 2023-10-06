/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SPIN_LOCK_HPP
#define KAGOME_SPIN_LOCK_HPP

#include <atomic>

namespace kagome::common {

  /**
   * Implements a spin lock synchronization primitive.
   *
   * Does busy-waiting over an atomic flag, thus the best application would
   * be for the cases when the lock is required for a very short amount of time
   * (to perform very few and finite computational steps under the lock).
   *
   * An advantage over std::mutex is locking without a call to system's kernel
   * and eliminating expensive context switches.
   *
   * Can be used with std::lock_guard:
   * @code
   * spin_lock mutex;
   * {
   *    std::lock_guard lock(mutex);
   *    // synchronized computations go here
   * }
   * @endcode
   */
  class spin_lock {  // the name styling is unified with std
   public:
    spin_lock() = default;
    spin_lock(const spin_lock &) = delete;
    spin_lock(spin_lock &&) = delete;
    spin_lock &operator=(const spin_lock &) = delete;
    spin_lock &operator=(spin_lock &&) = delete;

    /**
     * Acquires the lock.
     *
     * Blocks thread execution until the other thread releases the lock in a
     * busy-waiting manner.
     */
    void lock();

    /**
     * Releases the lock.
     */
    void unlock();

   private:
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
  };

}  // namespace kagome::common

#endif  // KAGOME_SPIN_LOCK_HPP
