#ifdef _WIN32
#include <winsock2.h>
#endif

#include "gtest/gtest.h"

#include "cpp_redis/cpp_redis"
#include "redispack/hash.h"

#include <exception>
#include <iostream>
#include <memory>
#include <string>

// cpp_redis
using cpp_redis::redis_client;
using cpp_redis::reply;

// redispack
using redispack::hash;

// std
using std::boolalpha;
using std::cerr;
using std::cout;
using std::exception;
using std::make_shared;
using std::string;

TEST(Main, Miscellaneous) {
    try {
        static constexpr auto KEY = 777;

        auto clientPtr = make_shared<redis_client>();
        clientPtr->connect();

        hash<int, string> h(clientPtr, "hash");

        cout << boolalpha << h.set(KEY, "Hello World!") << "\n";
        cout << boolalpha << h.exists(KEY) << "\n";

        h.get(KEY).match(
            [](const string &value) {
                cout << "(" << KEY << ": " << value << ")\n";
            },
            [] { cout << "(NIL)\n"; });

        h.setnx(888, "EightX3");

        for (const auto key : h.keys()) {
            cout << "Key: " << key << "\n";
        }

        for (const auto value : h.vals()) {
            cout << "Val: " << value << "\n";
        }

        for (const auto key_val : h.key_vals()) {
            cout << "(Key: " << key_val.first << ", Val: " << key_val.second << ")\n";
        }

        cout << "Length: " << h.len() << "\n";
        cout << boolalpha << h.del(KEY) << "\n";
        cout << "Length: " << h.len() << "\n";
    }
    catch (const exception &e) {
        cerr << "Error: " << e.what() << "\n";
        FAIL();
    }

    SUCCEED();
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
