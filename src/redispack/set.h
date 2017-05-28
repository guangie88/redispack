/**
 * Provides the important hash functions from redis, which makes it similar
 * to the cutdown version of std::unordered_set,
 * but with all the Redis set type goodness.
 *
 * @author Chen Weiguang
 */

#pragma once

#include "util.h"

#include "cpp_redis/cpp_redis"
#include "msgpack.hpp"
#include "rustfp/collect.h"
#include "rustfp/iter.h"
#include "rustfp/map.h"
#include "rustfp/option.h"

#include <cstddef>
#include <exception>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace redispack {

    // declaration section

    template <class T>
    class set {
    public:
        /** Alias to the T template type, which is the member type. */
        using value_type = T;

        /**
         * Constructs this instance with the given client connection and set key (name).
         */
        set(const redis_client_ptr &client_ptr, const std::string &name);

        /**
         * sadd
         * @param member member to add into the set
         * @param members other elements to add into the set
         * @return number of elements successfully added into the set
         */
        template <class... Ts>
        auto add(const T &member, const Ts &... members) -> size_t;

        /**
         * sadd
         * @param begin_t begin iterator of element to add into the set
         * @param end_t end iterator of element to add into the set
         * @return number of elements successfully added into the set
         */
        template <class TBeginIter, class TEndIter>
        auto add(const TBeginIter &begin_it, const TEndIter &end_it) -> size_t;

        /**
         * sadd
         * @param members members to add into the set
         * @return number of elements successfully added into the set
         */
        auto add(const std::vector<T> &members) -> size_t;

        /**
         * scard
         * @return number of elements in the given set
         */
        auto card() const -> size_t;

        /**
         * Clears the set.
         * @return number of removed elements in the given set
         */
        auto clear() -> size_t;

        /**
         * sdiff
         * @return subtraction result of *this and rhs
         */
        template <class Tx>
        auto diff(const set<Tx> &rhs) const -> std::unordered_set<T>;

        /**
         * sinter
         * @return intersection result of *this and rhs
         */
        template <class Tx>
        auto inter(const set<Tx> &rhs) const -> std::unordered_set<T>;

        /**
         * sismember
         * @return boolean indicating if the given member is in the set
         */
        auto is_member(const T &member) const -> bool;

        /**
         * smembers
         * @return retrieve all the members in the set
         */
        auto members() const -> std::unordered_set<T>;

        /**
         * srem
         * @param member member to remove from the set
         * @param members other members to remove from the set
         * @return number of elements successfully removed from the set
         */
        template <class... Ts>
        auto rem(const T &member, const Ts &... members) -> size_t;

        /**
         * sadd
         * @param begin_t begin iterator of element to remove from the set
         * @param end_t end iterator of element to remove from the set
         * @return number of elements successfully removed from the set
         */
        template <class TBeginIter, class TEndIter>
        auto rem(const TBeginIter &begin_it, const TEndIter &end_it) -> size_t;

        /**
         * srem
         * @param members members to remove from the set
         * @return number of elements successfully removed into the set
         */
        auto rem(const std::vector<T> &members) -> size_t;

        /**
         * sunion
         * @return union result of *this and rhs
         */
        template <class Tx>
        auto union_(const set<Tx> &rhs) const -> std::unordered_set<T>;

    private:
        /** Holds a shared ownership to access the database. */
        mutable redis_client_ptr client_ptr;

        /** Set key (name). */
        std::string name;
    };

    // implementation section

    namespace details {
        auto add_impl(
            redis_client_ptr &client_ptr,
            const std::string &name,
            const std::vector<std::string> &member_strs) -> size_t {

            size_t added_count = 0;

            client_ptr->sadd(name, member_strs,
                [&added_count](cpp_redis::reply &r) {
                    if (r.is_integer()) {
                        added_count = r.as_integer();
                    }
                });

            details::sync_commit(client_ptr);
            return added_count;
        }

        auto rem_impl(
            redis_client_ptr &client_ptr,
            const std::string &name,
            const std::vector<std::string> &member_strs) -> size_t {

            size_t remove_count = 0;

            client_ptr->srem(name, member_strs,
                [&remove_count](cpp_redis::reply &r) {
                    if (r.is_integer()) {
                        remove_count = r.as_integer();
                    }
                });

            details::sync_commit(client_ptr);
            return remove_count;
        }
    }

    template <class T>
    set<T>::set(const redis_client_ptr &client_ptr, const std::string &name) :
        client_ptr(client_ptr),
        name(name) {

    }

    template <class T>
    template <class... Ts>
    auto set<T>::add(const T &member, const Ts &... members) -> size_t {
        const auto member_strs = details::str_vectorize(member, members...);
        return details::add_impl(client_ptr, name, member_strs);
    }

    template <class T>
    template <class TBeginIter, class TEndIter>
    auto set<T>::add(const TBeginIter &begin_it, const TEndIter &end_it) -> size_t {
        return add(std::vector<T>(begin_it, end_it));
    }

    template <class T>
    auto set<T>::add(const std::vector<T> &members) -> size_t {
        const auto member_strs = ::rustfp::iter(members)
            | ::rustfp::map([](const T &member) {
                return details::encode_into_str(member);
            })
            | ::rustfp::collect<std::vector<std::string>>();

        return details::add_impl(client_ptr, name, member_strs);
    }

    template <class T>
    auto set<T>::card() const -> size_t {
        size_t cardinality = 0;

        client_ptr->scard(name,
            [&cardinality](cpp_redis::reply &r) {
                if (r.is_integer()) {
                    cardinality = r.as_integer();
                }
            });

        details::sync_commit(client_ptr);
        return cardinality;
    }

    template <class T>
    auto set<T>::clear() -> size_t {
        // this is more inefficient because of
        // packing and unpacking of msgpack values
        const auto mems_set = members();

        const auto mems = ::rustfp::iter(mems_set)
            | ::rustfp::collect<std::vector<T>>();
            
        return rem(mems);
    }

    template <class T>
    template <class Tx>
    auto set<T>::diff(const set<Tx> &rhs) const -> std::unordered_set<T> {
        std::unordered_set<T> mems;

        client_ptr->sdiff(std::vector<std::string>{name, rhs.name},
            [&mems](cpp_redis::reply &r) {
                if (r.is_array()) {
                    for (const auto &sub_r : r.as_array()) {
                        if (sub_r.is_bulk_string()) {
                            auto mem_opt = details::decode_from_str<T>(
                                sub_r.as_string());

                            std::move(mem_opt).match_some(
                                [&mems](T &&mem) {
                                    mems.insert(std::move(mem));
                                });
                        }
                    }
                }
            });

        details::sync_commit(client_ptr);
        return mems;
    }

    template <class T>
    template <class Tx>
    auto set<T>::inter(const set<Tx> &rhs) const -> std::unordered_set<T> {
        std::unordered_set<T> mems;

        client_ptr->sinter(std::vector<std::string>{name, rhs.name},
            [&mems](cpp_redis::reply &r) {
                if (r.is_array()) {
                    for (const auto &sub_r : r.as_array()) {
                        if (sub_r.is_bulk_string()) {
                            auto mem_opt = details::decode_from_str<T>(
                                sub_r.as_string());

                            std::move(mem_opt).match_some(
                                [&mems](T &&mem) {
                                    mems.insert(std::move(mem));
                                });
                        }
                    }
                }
            });

        details::sync_commit(client_ptr);
        return mems;
    }

    template <class T>
    auto set<T>::is_member(const T &member) const -> bool {
        const auto member_str = details::encode_into_str(member);
        auto is_member_flag = false;

        client_ptr->sismember(name, member_str,
            [&is_member_flag](cpp_redis::reply &r) {
                if (r.is_integer() && r.as_integer() == 1) {
                    is_member_flag = true;
                }
            });

        details::sync_commit(client_ptr);
        return is_member_flag;
    }

    template <class T>
    auto set<T>::members() const -> std::unordered_set<T> {
        std::unordered_set<T> mems;

        client_ptr->smembers(name,
            [&mems](cpp_redis::reply &r) {
                if (r.is_array()) {
                    for (const auto &sub_r : r.as_array()) {
                        if (sub_r.is_bulk_string()) {
                            auto mem_opt = details::decode_from_str<T>(
                                sub_r.as_string());

                            std::move(mem_opt).match_some(
                                [&mems](T &&mem) {
                                    mems.insert(std::move(mem));
                                });
                        }
                    }
                }
            });

        details::sync_commit(client_ptr);
        return mems;
    }

    template <class T>
    template <class... Ts>
    auto set<T>::rem(const T &member, const Ts &... members) -> size_t {
        const auto member_strs = details::str_vectorize(member, members...);
        return details::rem_impl(client_ptr, name, member_strs);
    }

    template <class T>
    template <class TBeginIter, class TEndIter>
    auto set<T>::rem(const TBeginIter &begin_it, const TEndIter &end_it) -> size_t {
        return rem(std::vector<T>(begin_it, end_it));
    }

    template <class T>
    auto set<T>::rem(const std::vector<T> &members) -> size_t {
        const auto member_strs = ::rustfp::iter(members)
            | ::rustfp::map([](const T &member) {
                return details::encode_into_str(member);
            })
            | ::rustfp::collect<std::vector<std::string>>();

        return details::rem_impl(client_ptr, name, member_strs);
    }

    template <class T>
    template <class Tx>
    auto set<T>::union_(const set<Tx> &rhs) const -> std::unordered_set<T> {
        std::unordered_set<T> mems;

        client_ptr->sunion(std::vector<std::string>{name, rhs.name},
            [&mems](cpp_redis::reply &r) {
                if (r.is_array()) {
                    for (const auto &sub_r : r.as_array()) {
                        if (sub_r.is_bulk_string()) {
                            auto mem_opt = details::decode_from_str<T>(
                                sub_r.as_string());

                            std::move(mem_opt).match_some(
                                [&mems](T &&mem) {
                                    mems.insert(std::move(mem));
                                });
                        }
                    }
                }
            });

        details::sync_commit(client_ptr);
        return mems;
    }
}