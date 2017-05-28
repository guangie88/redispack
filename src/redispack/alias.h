#pragma once

#include "cpp_redis/redis_client.hpp"
#include "cpp_redis/redis_error.hpp"

#include <memory>

namespace redispack {
    /** Alias to cpp_redis::redis_error. */
    using redis_error = ::cpp_redis::redis_error;

    /** Alias to cpp_redis::redis_client. */
    using redis_client = ::cpp_redis::redis_client;

    /** Alias to the common form of redis_client shared ownership pointer. **/
    using redis_client_ptr = std::shared_ptr<redis_client>;
}