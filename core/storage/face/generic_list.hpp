/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_QUEUE_HPP
#define KAGOME_QUEUE_HPP

namespace kagome::face {

    /// An interface for a generic FIFO container
    template <typename T>
    class Queue {
    public:
        /**
         * Put an element to the tail of the queue
         * @param t the element
         */
        virtual void push(T &&t) = 0;
        virtual void push(const T &t) = 0;

        /**
         * Get the head element of the queue and then remove it
         * @return the element
         */
        virtual T pop() = 0;

        /** Get the head element of the queue
         * @return the const ref to the element
         */
        virtual const T& peek() const = 0;

        /**
         * Tell whether queue is empty
         */
        virtual bool isEmpty() const = 0;
    };

}

#endif //KAGOME_QUEUE_HPP
