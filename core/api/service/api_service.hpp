/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_SERVICE_HPP
#define KAGOME_CORE_API_SERVICE_HPP

#include <functional>

#include "api/transport/listener.hpp"

namespace kagome::api {

  class JRPCProcessor;

  class ApiService {
   public:
    template <class T>
    using sptr = std::shared_ptr<T>;

    /**
     * @brief constructor
     * @param context reference to io context
     * @param listener sptr to listener instance
     * @param processor sptr to json-rpc processor instance
     */
    ApiService(std::shared_ptr<Listener> listener,
               std::shared_ptr<JRPCProcessor> processor);

    virtual ~ApiService() = default;
    /**
     * @brief starts service
     */
    virtual void start();

    /**
     * @brief stops service
     */
    virtual void stop();

   private:
    sptr<Listener> listener_;        ///< endpoint listener
    sptr<JRPCProcessor> processor_;  ///< json-rpc processor
  };
}  // namespace kagome::api

#endif  // KAGOME_CORE_API_SERVICE_HPP
