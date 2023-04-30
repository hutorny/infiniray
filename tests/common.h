#pragma once
#include <iostream>

static inline int expect_true(bool condition, std::string_view strcond, std::string_view file, int line) {
    if (!condition) {
        std::cerr << file << ':' << line << ": FAILED " << strcond << '\n';
        return 1;
    }
    return 0;
}

static inline int expect_match_(std::string_view actual, std::string_view expected, std::string_view file, int line) {
    if (actual != expected) {
        std::cerr << file << ':' << line << ": FAILED, ACTUAL\n" << actual << "\nEXPECTED\n" << expected << '\n';
        return 1;
    }
    return 0;
}

template<typename Actual, typename Expected>
static inline int expect_match_(Actual&& actual, Expected&& expected, std::string_view file, int line) {
    if (!(actual == expected)) {
        std::cerr << file << ':' << line << ": FAILED, ACTUAL " << actual << " != " << expected << " EXPECTED\n";
        return 1;
    }
    return 0;
}

#define expect(cond) expect_true(cond, #cond, __FILE__, __LINE__)
#define expect_success(cond) expect_success_(cond, #cond, __FILE__, __LINE__)
#define expect_match(actual, expected) expect_match_(actual, expected, __FILE__, __LINE__)
