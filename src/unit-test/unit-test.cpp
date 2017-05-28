#ifdef _WIN32
#include <winsock2.h>
#endif

#include "gtest/gtest.h"

#include "redispack/connection.h"
#include "redispack/hash.h"
#include "redispack/set.h"

#include <algorithm>
#include <array>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

// redispack 
using redispack::hash;
using redispack::make_and_connect;
using redispack::set;

// std
using std::all_of;
using std::array;
using std::boolalpha;
using std::cerr;
using std::cout;
using std::exception;
using std::find;
using std::make_shared;
using std::string;

TEST(Hash, MakeAndConnect) {
    auto client_ptr = make_and_connect().unwrap_unchecked();
    EXPECT_TRUE(client_ptr->is_connected());
}

TEST(Hash, SetExistsDel) {
    auto client_ptr = make_and_connect().unwrap_unchecked();
    hash<string, string> h(client_ptr, "hash_set_exists_del");

    // not testing whether the entry does exist before or not
    h.del("a");
    EXPECT_FALSE(h.exists("a"));

    h.set("a", "AAA");
    EXPECT_TRUE(h.exists("a"));
    
    const auto opt = h.get("a");
    EXPECT_TRUE(opt.is_some());
    EXPECT_EQ("AAA", opt.get_unchecked());
    
    EXPECT_TRUE(h.del("a"));
    EXPECT_FALSE(h.exists("a"));

    const auto opt_none = h.get("a");
    EXPECT_TRUE(opt_none.is_none());
}

TEST(Hash, SetExistsGet) {
    auto client_ptr = make_and_connect().unwrap_unchecked();
    hash<int, string> h(client_ptr, "hash_set_exists_get");

    h.set(777, "Hello World!");
    EXPECT_TRUE(h.exists(777));
    
    const auto opt = h.get(777);
    EXPECT_TRUE(opt.is_some());
    EXPECT_EQ("Hello World!", opt.get_unchecked());
}

TEST(Hash, KeyVals) {
    auto client_ptr = make_and_connect().unwrap_unchecked();
    hash<int, string> h(client_ptr, "hash_key_vals");

    const array<int, 3> keys{8, 2, 77};
    const array<string, 3> values{"Eight", "Two", "Seven"};

    // delete all entries first
    for (const auto key : h.keys()) {
        EXPECT_TRUE(h.del(key));
    }

    for (size_t i = 0; i < keys.size(); ++i) {
        EXPECT_TRUE(h.setnx(keys[i], values[i]));
    }

    EXPECT_TRUE(all_of(keys.cbegin(), keys.cend(), [&h](const auto key) {
        const auto h_keys = h.keys();
        return h_keys.find(key) != h_keys.cend();
    }));

    EXPECT_TRUE(all_of(values.cbegin(), values.cend(), [&h](const auto &value) {
        const auto h_vals = h.vals();
        return find(h_vals.cbegin(), h_vals.cend(), value) != h_vals.cend();
    }));

    const auto range = {0, 1, 2};
    
    EXPECT_TRUE(all_of(range.begin(), range.end(), [&h, &keys, &values](const auto index) {
        const auto key = keys[index];
        const auto &value = values[index];
        return h.key_vals().at(key) == value;
    }));
}

TEST(Set, AddIsMemberRemOne) {
    auto client_ptr = make_and_connect().unwrap_unchecked();

    set<string> s(client_ptr, "set_add_is_member_rem_one");
    s.clear();

    const auto added_count = s.add("Hello World!");
    EXPECT_EQ(1, added_count);
    EXPECT_EQ(1, s.card());

    EXPECT_FALSE(s.is_member("hello world!"));
    EXPECT_FALSE(s.is_member("foobar"));
    EXPECT_TRUE(s.is_member("Hello World!"));

    const auto remove_count = s.rem("Hello World!");
    EXPECT_EQ(1, remove_count);
    EXPECT_EQ(0, s.card());
}

TEST(Set, Members) {
    auto client_ptr = make_and_connect().unwrap_unchecked();

    set<double> s(client_ptr, "set_members");
    s.clear();

    const array<double, 4> add_arr{0.0, 5.0, 0.0, 3.0};

    EXPECT_EQ(4, s.add(3.5, 0.0, 1.25, 10.0, 0.0));
    EXPECT_EQ(2, s.add(add_arr.cbegin(), add_arr.cend()));
    EXPECT_EQ(6, s.card());

    const auto ms1 = s.members();
    EXPECT_EQ(6, ms1.size());
    EXPECT_TRUE(ms1.find(3.5) != ms1.cend());
    EXPECT_TRUE(ms1.find(0.0) != ms1.cend());
    EXPECT_TRUE(ms1.find(1.25) != ms1.cend());
    EXPECT_TRUE(ms1.find(10.0) != ms1.cend());
    EXPECT_TRUE(ms1.find(5.0) != ms1.cend());
    EXPECT_TRUE(ms1.find(3.0) != ms1.cend());
    EXPECT_FALSE(ms1.find(3.25) != ms1.cend());

    EXPECT_EQ(3, s.rem(1.25, 0.0, 1.25, 3.5));
    EXPECT_EQ(3, s.card());

    const array<double, 4> rem_arr{1.25, 1.0, 0.0, 3.0};
    EXPECT_EQ(1, s.rem(rem_arr.cbegin(), rem_arr.cend()));
    EXPECT_EQ(2, s.card());

    const auto ms2 = s.members();
    EXPECT_EQ(2, ms2.size());
    EXPECT_FALSE(ms2.find(3.5) != ms2.cend());
    EXPECT_FALSE(ms2.find(0.0) != ms2.cend());
    EXPECT_FALSE(ms2.find(1.25) != ms2.cend());
    EXPECT_TRUE(ms2.find(10.0) != ms2.cend());
    EXPECT_TRUE(ms2.find(5.0) != ms2.cend());
    EXPECT_FALSE(ms2.find(3.0) != ms2.cend());
    EXPECT_FALSE(ms2.find(3.25) != ms2.cend());

    EXPECT_EQ(2, s.clear());

    const auto ms3 = s.members();
    EXPECT_EQ(0, ms3.size());
}

TEST(Set, Diff) {
    auto client_ptr = make_and_connect().unwrap_unchecked();

    set<string> lhs(client_ptr, "set_diff1");
    lhs.clear();

    EXPECT_EQ(4, lhs.add("Hello", "how", "are", "you"));
    EXPECT_EQ(4, lhs.card());

    set<string> rhs(client_ptr, "set_diff2");
    rhs.clear();

    EXPECT_EQ(3, rhs.add("how", "are", "these?"));
    EXPECT_EQ(3, rhs.card());
    
    const auto diff = lhs.diff(rhs);
    EXPECT_EQ(2, diff.size());
    EXPECT_TRUE(diff.find("Hello") != diff.cend());
    EXPECT_TRUE(diff.find("you") != diff.cend());
    EXPECT_FALSE(diff.find("these?") != diff.cend());
}

TEST(Set, Inter) {
    auto client_ptr = make_and_connect().unwrap_unchecked();

    set<string> lhs(client_ptr, "set_inter1");
    lhs.clear();

    EXPECT_EQ(4, lhs.add("Hello", "how", "are", "you"));
    EXPECT_EQ(4, lhs.card());

    set<string> rhs(client_ptr, "set_inter2");
    rhs.clear();

    EXPECT_EQ(3, rhs.add("how", "are", "these?"));
    EXPECT_EQ(3, rhs.card());
    
    const auto diff = lhs.inter(rhs);
    EXPECT_EQ(2, diff.size());
    EXPECT_TRUE(diff.find("how") != diff.cend());
    EXPECT_TRUE(diff.find("are") != diff.cend());
    EXPECT_FALSE(diff.find("Hello") != diff.cend());
    EXPECT_FALSE(diff.find("you") != diff.cend());
    EXPECT_FALSE(diff.find("these?") != diff.cend());
}

TEST(Set, Union) {
    auto client_ptr = make_and_connect().unwrap_unchecked();

    set<string> lhs(client_ptr, "set_union1");
    lhs.clear();

    EXPECT_EQ(4, lhs.add("Hello", "how", "are", "you"));
    EXPECT_EQ(4, lhs.card());

    set<string> rhs(client_ptr, "set_union2");
    rhs.clear();

    EXPECT_EQ(3, rhs.add("how", "are", "these?"));
    EXPECT_EQ(3, rhs.card());
    
    const auto diff = lhs.union_(rhs);
    EXPECT_EQ(5, diff.size());
    EXPECT_TRUE(diff.find("how") != diff.cend());
    EXPECT_TRUE(diff.find("are") != diff.cend());
    EXPECT_TRUE(diff.find("Hello") != diff.cend());
    EXPECT_TRUE(diff.find("you") != diff.cend());
    EXPECT_TRUE(diff.find("these?") != diff.cend());
    EXPECT_FALSE(diff.find("hello") != diff.cend());
}

int main(int argc, char * argv[]) {

#ifdef _WIN32
    //! Windows netword DLL init
    WORD version = MAKEWORD(2, 2);
    WSADATA data;

    if (WSAStartup(version, &data) != 0) {
        cerr << "WSAStartup() failure\n";
        return 127;
    }
#endif

    testing::InitGoogleTest(&argc, argv);
    const auto exit_code = RUN_ALL_TESTS();

#ifdef _WIN32
    WSACleanup();
#endif

    return exit_code;
}
