#ifdef _WIN32
#include <winsock2.h>
#endif

#include "gtest/gtest.h"

#include "redispack/connection.h"
#include "redispack/hash.h"

#include <algorithm>
#include <array>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

// redispack 
using redispack::hash;
using redispack::make_and_connect;

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

//     h.get(KEY).match(
//         [](const string &value) {
//             cout << "(" << KEY << ": " << value << ")\n";
//         },
//         [] { cout << "(NIL)\n"; });
// 
//     h.setnx(888, "EightX3");
// 
//     for (const auto key : h.keys()) {
//         cout << "Key: " << key << "\n";
//     }
// 
//     for (const auto value : h.vals()) {
//         cout << "Val: " << value << "\n";
//     }
// 
//     for (const auto key_val : h.key_vals()) {
//         cout << "(Key: " << key_val.first << ", Val: " << key_val.second << ")\n";
//     }
// 
//     cout << "Length: " << h.len() << "\n";
//     cout << boolalpha << h.del(KEY) << "\n";
//     cout << "Length: " << h.len() << "\n";
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
