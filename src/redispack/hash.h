/**
 * Provides the important hash functions from redis, which makes it similar
 * to the cutdown version of std::map, but with all the Redis hash type goodness.
 *
 * @author Chen Weiguang
 */

#pragma once

#include "cpp_redis/cpp_redis"
#include "msgpack.hpp"
#include "rustfp/option.h"

#include <cstddef>
#include <exception>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace redispack
{
    // declaration section

    namespace details
    {
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
    }

    /** 
     * Provides hash like functionalities from redis.
     *
     * All the data is stored in the redis server.
     */
    template <class K, class V>
    class hash
    {
    public:
        /**
         * Alias to the K template type, which is the key.
         */
        using key_t = K;

        /**
         * Alias to the V template type, which is the value.
         */
        using value_t = V;

        /**
         * Constructs this instance with the given client connection and hash key (name).
         */
        hash(const std::shared_ptr<cpp_redis::redis_client> &client_ptr, const std::string &name);

        /** 
         * Performs hdel command.
         *
         * @return true if the key used succeeds to delete an entry.
         */
        auto del(const K &key) -> bool;

        /**
         * Performs hexists command.
         *
         * @return true if the entry exists.
         */
        auto exists(const K &key) const -> bool;

        /** 
         * Performs the hget command.
         *
         * @return Some(value) if the entry exists, otherwise None.
         */
        auto get(const K &key) const -> rustfp::Option<V>;

        /**
         * Performs the hkeys command.
         *
         * @return keys from database, as a copy.
         */
        auto keys() const -> std::unordered_set<K>;

        /** 
         * Performs the hlen command.
         *
         * @return number of entries in the hash.
         */
        auto len() const -> size_t;

        /**
         * Performs the hset command.
         *
         * @return true if the entry is new, false if the entry was updated.
         */
        auto set(const K &key, const V value) -> bool;

        /**
         * Performs the hsetnx command.
         *
         * @return true if the entry is new, false if another entry with same key exists
         * and no update will be performed.
         */
        auto setnx(const K &key, const V value) -> bool;

        /**
         * Performs the hvals command.
         *
         * @return list of values as a copy.
         */
        auto vals() const -> std::vector<V>;

        /**
         * Additional functionality by zipping the key and value side-by-side.
         *
         * @return keys with corresponding values as a copy.
         */
        auto key_vals() const -> std::unordered_map<K, V>;

    private:
        /**
         * Performs sync commit to database. 
         */
        void sync_commit() const;

        /**
         * Holds a shared ownership to access the database.
         */
        mutable std::shared_ptr<cpp_redis::redis_client> client_ptr;

        /** 
         * Hash key (name).
         */
        std::string name;
    };

    // implementation section

    namespace details
    {
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
    }

    template <class K, class V>
    hash<K, V>::hash(const std::shared_ptr<cpp_redis::redis_client> &client_ptr, const std::string &name) :
        client_ptr(client_ptr),
        name(name) {

    }

    template <class K, class V>
    auto hash<K, V>::del(const K &key) -> bool {
        const auto key_strs = std::vector<std::string>{details::encode_into_str(key)};
        bool deleted = false;

        client_ptr->hdel(name, key_strs,
            [&deleted](cpp_redis::reply &r) {
                if (r.is_integer() && r.as_integer() > 0) {
                    deleted = true;
                }
            });

        sync_commit();
        return deleted;
    }

    template <class K, class V>
    auto hash<K, V>::exists(const K &key) const -> bool {
        const auto key_str = details::encode_into_str(key);
        bool is_present = false;

        client_ptr->hexists(name, key_str,
            [&is_present](cpp_redis::reply &r) {
                static constexpr auto CONTAINS_FIELD_RET_VAL = 1;

                if (r.is_integer() && r.as_integer() == CONTAINS_FIELD_RET_VAL) {
                    is_present = true;
                }
            });

        sync_commit();
        return is_present;
    }

    
    template <class K, class V>
    auto hash<K, V>::get(const K &key) const -> rustfp::Option<V> {
        const auto key_str = details::encode_into_str(key);
        rustfp::Option<V> value_opt = rustfp::None;

        client_ptr->hget(name, key_str,
            [&value_opt](cpp_redis::reply &r) {
                if (r.is_bulk_string()) {
                    value_opt = details::decode_from_str<V>(r.as_string());
                }
            });

        sync_commit();
        return std::move(value_opt);
    }

    template <class K, class V>
    auto hash<K, V>::keys() const -> std::unordered_set<K> {
        std::unordered_set<K> keys;

        client_ptr->hkeys(name,
            [&keys](cpp_redis::reply &r) {
                if (r.is_array()) {
                    for (const auto &sub_r : r.as_array()) {
                        if (sub_r.is_bulk_string()) {
                            auto key_opt = details::decode_from_str<K>(sub_r.as_string());

                            std::move(key_opt).match_some(
                                [&keys](K &&key) {
                                    keys.insert(std::move(key));
                                });
                        }
                    }
                }
            });

        sync_commit();
        return keys;
    }

    template <class K, class V>
    auto hash<K, V>::len() const -> size_t {
        size_t length = 0;

        client_ptr->hlen(name,
            [&length](cpp_redis::reply &r) {
                if (r.is_integer()) {
                    length = static_cast<size_t>(r.as_integer());
                }
            });

        sync_commit();
        return length;
    }

    template <class K, class V>
    auto hash<K, V>::set(const K &key, const V value) -> bool {
        const auto key_str = details::encode_into_str(key);
        const auto val_str = details::encode_into_str(value);

        bool is_new_field = false;

        client_ptr->hset(name, key_str, val_str,
            [&is_new_field](cpp_redis::reply &r) {
                static constexpr auto IS_NEW_FIELD_RET_VAL = 1;

                if (r.is_integer() && r.as_integer() == IS_NEW_FIELD_RET_VAL) {
                    is_new_field = true;
                }
            });

        sync_commit();
        return is_new_field;
    }

    template <class K, class V>
    auto hash<K, V>::setnx(const K &key, const V value) -> bool {
        const auto key_str = details::encode_into_str(key);
        const auto val_str = details::encode_into_str(value);

        bool is_new_field = false;

        client_ptr->hsetnx(name, key_str, val_str,
            [&is_new_field](cpp_redis::reply &r) {
                static constexpr auto IS_NEW_FIELD_RET_VAL = 1;

                if (r.is_integer() && r.as_integer() == IS_NEW_FIELD_RET_VAL) {
                    is_new_field = true;
                }
            });

        sync_commit();
        return is_new_field;
    }

    template <class K, class V>
    auto hash<K, V>::vals() const -> std::vector<V> {
        std::vector<V> values;

        client_ptr->hvals(name,
            [&values](cpp_redis::reply &r) {
                if (r.is_array()) {
                    for (const auto &sub_r : r.as_array()) {
                        if (sub_r.is_bulk_string()) {
                            auto value_opt = details::decode_from_str<V>(sub_r.as_string());

                            std::move(value_opt).match_some(
                                [&values](V &&value) {
                                    values.push_back(std::move(value));
                                });
                        }
                    }
                }
            });

        sync_commit();
        return values;
    }

    template <class K, class V>
    auto hash<K, V>::key_vals() const -> std::unordered_map<K, V> {
        const auto ks = keys();

        std::unordered_map<K, V> key_vals;
        key_vals.reserve(ks.size());

        for (const auto &k : ks) {
            get(k).match_some(
                [&k, &key_vals](V &&value) {
                    key_vals.emplace(k, std::move(value));
                });
        }

        // nothing to commit here the suboperations will do the commit
        return key_vals;
    }

    template <class K, class V>
    void hash<K, V>::sync_commit() const {
        client_ptr->sync_commit();
    }
}
