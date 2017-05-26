/**
 * Contains convenient client connection functionality.
 *
 * @author Chen Weiguang
 */

#pragma once

#include "cpp_redis/redis_client.hpp"
#include "cpp_redis/redis_error.hpp"
#include "rustfp/result.h"

#include <cstddef>
#include <exception>
#include <memory>
#include <string>

namespace redispack {

    // declaration section

    namespace details {
        static constexpr auto DEFAULT_HOST = "127.0.0.1";
        static constexpr auto DEFAULT_PORT = 6379;
    }

    /** Alias to cpp_redis::redis_error. */
    using redis_error = ::cpp_redis::redis_error;

    /** Alias to cpp_redis::redis_client */
    using redis_client = ::cpp_redis::redis_client;

    /**
     * Creates and immediately connects the client to the
     * server.
     *
     * @param hostname of the server, defaults to 127.0.0.1
     * @param port of the server, defaults to 6379
     * @return client shared pointer wrapped in Ok<std::shared_ptr>,
     * any exception is caught and returned as Err<std::unique_ptr<std::exception>>
     */
    auto make_and_connect(
        const std::string &host = details::DEFAULT_HOST,
        const size_t port = details::DEFAULT_PORT) noexcept
        -> rustfp::Result<std::shared_ptr<redis_client>, std::unique_ptr<std::exception>>;

    // implementation section

    inline auto make_and_connect(
        const std::string &host,
        const size_t port) noexcept
        -> rustfp::Result<std::shared_ptr<redis_client>, std::unique_ptr<std::exception>> {

        try {
            auto client_ptr = std::make_shared<redis_client>(); 
            client_ptr->connect(host, port);
            return rustfp::Ok(std::move(client_ptr));
        }
        catch (const std::exception &e) {
            return rustfp::Err(std::make_unique<std::exception>(e));
        }
    }
}
