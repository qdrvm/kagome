/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "pvm/native/linux.hpp"
#include "pvm/types.hpp"

namespace kagome::pvm::native::linux {

  /**
   * @brief Structure for memory mapping (mmap) on Linux.
   *
   * This class manages memory mapping to the process's virtual address space
   * using the `mmap` system call and provides functions for mapping and
   * unmapping memory.
   */
  class Mmap final {
    void *pointer;  ///< Pointer to the beginning of the mapped memory region.
    size_t length;  ///< Size of the mapped memory region.

    /**
     * @brief Constructor.
     *
     * Constructs an Mmap object with the specified pointer and length.
     *
     * @param p Pointer to the beginning of the mapped memory region.
     * @param l Length of the mapped memory region.
     */
    Mmap(void *p, size_t l) : pointer(p), length(l) {}

    /**
     * @brief Internal method to unmap the memory.
     *
     * Implements the logic to unmap the memory and reset the pointer.
     *
     * @return The result of the unmapping operation.
     */
    Result<void> unmap_inplace() {
      if (length > 0) {
        OUTCOME_TRY(sys_munmap(pointer, length));
        length = 0;
        pointer = nullptr;
      }
      return outcome::success();
    }

   public:
    /**
     * @brief Deleted copy constructor.
     *
     * Prevents copying of the Mmap object.
     */
    Mmap(const Mmap &) = delete;

    /**
     * @brief Move constructor for Mmap
     *
     * This constructor transfers ownership of resources from another `Mmap`
     * object to the current one. After the move, the `other` object is left in
     * a valid but unspecified state, with its pointer set to `nullptr` and
     * length set to `0`.
     *
     * @param other The `Mmap` object from which resources are moved
     */
    Mmap(Mmap &&other) : pointer(other.pointer), length(other.length) {
      other.pointer = nullptr;
      other.length = 0;
    }

    /**
     * @brief Deleted copy assignment operator.
     *
     * Prevents assignment between Mmap objects.
     */
    Mmap &operator=(const Mmap &) = delete;

    /**
     * @brief Creates a new Mmap object with the specified parameters.
     *
     * @param address The address in the virtual memory space.
     * @param length The size of the memory to map.
     * @param protection Memory protection flags (e.g., read, write).
     * @param flags Memory mapping flags.
     * @param fd File descriptor (optional).
     * @param offset Offset in the file for mapping.
     *
     * @return The result of the memory mapping creation.
     */
    static Result<Mmap> create(void *address,
                               size_t length,
                               uint32_t protection,
                               uint32_t flags,
                               Opt<Fd> fd,
                               uint64_t offset) {
      OUTCOME_TRY(pointer,
                  sys_mmap(address, length, protection, flags, fd, offset));
      return outcome::success(Mmap(pointer, length));
    }

    /**
     * @brief Destructor.
     *
     * Frees resources and unmaps the memory when the object is destroyed.
     */
    ~Mmap() {
      std::ignore = unmap_inplace();
    }

    /**
     * @brief Unmaps the mapped memory.
     *
     * Unmaps the memory if it was previously mapped.
     *
     * @return The result of the unmapping operation.
     */
    Result<void> unmap() {
      return unmap_inplace();
    }

    /**
     * @brief Retrieves a pointer to the mapped data.
     *
     * Returns a pointer to the beginning of the mapped memory region.
     *
     * @return A pointer to the data.
     */
    void *data() {
      return pointer;
    }

    /**
     * @brief Retrieves a constant pointer to the mapped data.
     *
     * Returns a constant pointer to the beginning of the mapped memory region.
     *
     * @return A constant pointer to the data.
     */
    const void *data() const {
      return pointer;
    }

    /**
     * @brief Retrieves a span of memory.
     *
     * Returns a `std::span<uint8_t>` that provides access to the data as a byte
     * array.
     *
     * @return A span of data.
     */
    std::span<const uint8_t> slice() const {
      return std::span<const uint8_t>(
          (const uint8_t *)pointer,
          std::span<const uint8_t>::size_type(length));
    }

    /**
     * @brief Retrieves the size of the mapped memory region.
     *
     * Returns the size of the mapped memory region in bytes.
     *
     * @return The size of the memory region.
     */
    size_t size() const {
      return length;
    }
  };

}  // namespace kagome::pvm::native::linux
