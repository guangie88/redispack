/**
 * Provides common functions across the header files.
 *
 * @author Chen Weiguang
 */

#pragma once

#include "alias.h"

#include "msgpack.hpp"
#include "rustfp/option.h"

#include <string>
#include <utility>
#include <vector>

namespace redispack {

    // declaration section

    namespace details {
        /**
         * Performs sync commit to database. 
         */
        void sync_commit(redis_client_ptr &client_ptr);

        /** 
         * Encodes given value into string by packing into msgpack format.
         */
        template <class V>
        auto encode_into_str(const V &value) -> std::string;

        /**
         * Decodes by unpacking from given string into possibly the actual value.
         */
        template <class V>
        auto decode_from_str(const std::string &str) -> rustfp::Option<V>;

        /**
         * Pack variadic template into vector form implementation, base case.
         */
        auto str_vectorize_impl(std::vector<std::string> &&vec)
            -> std::vector<std::string>;

        /**
         * Pack variadic template into vector form implementation.
         */
        template <class V, class... Vs>
        auto str_vectorize_impl(std::vector<std::string> &&vec, V &&v, Vs &&... vs)
            -> std::vector<std::string>;

        /**
         * Pack variadic template into vector form.
         */
        template <class V, class... Vs>
        auto str_vectorize(V &&v, Vs &&... vs) -> std::vector<std::string>;
    }

    // implementation section

    namespace details {
        inline void sync_commit(redis_client_ptr &client_ptr) {
            client_ptr->sync_commit();
        }

        template <class V>
        auto encode_into_str(const V &value) -> std::string {
            std::stringstream buf;
            ::msgpack::pack(buf, value);
            return buf.str();
        }

        template <class V>
        auto decode_from_str(const std::string &str) -> rustfp::Option<V> {
            try {
                V obj;

                ::msgpack::unpack(str.data(), str.size())
                    .get()
                    .convert(obj);

                return rustfp::Some(std::move(obj));
            }
            catch (const std::exception &e) {
                return rustfp::None;
            }
        }

        inline auto str_vectorize_impl(std::vector<std::string> &&vec)
            -> std::vector<std::string> {

            return std::move(vec);
        }

        template <class V, class... Vs>
        auto str_vectorize_impl(std::vector<std::string> &&vec, V &&v, Vs &&... vs)
            -> std::vector<std::string> {

            vec.push_back(encode_into_str(std::forward<V>(v)));
            return str_vectorize_impl(std::move(vec), std::forward<Vs>(vs)...);
        }

        template <class V, class... Vs>
        auto str_vectorize(V &&v, Vs &&... vs) -> std::vector<std::string> {
            std::vector<std::string> vec;
            vec.reserve(1 + sizeof...(Vs));

            return str_vectorize_impl(
                std::move(vec),
                std::forward<V>(v),
                std::forward<Vs>(vs)...);
        }
    }
}