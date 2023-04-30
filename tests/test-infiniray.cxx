/*
 * Copyright (C) 2023 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * infiniray-test.h - Infinite Array tests
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */

#include <iostream>
#include <algorithm>
#include <vector>
#include <infiniray.h>
#include "common.h"

using namespace std;
#pragma GCC diagnostic ignored "-Wfloat-equal"

struct test {
    test() { defaulted++; }
    test(long v) : value{v} { valued++; }
    test(const test & v) : value{v.value} { copied++; }
    test(test&& v) : value{v.value} { moved++; }
    test& operator=(const test& v) { value = v.value; return *this; }
    test& operator=(test&& v) { value = v.value; return *this; }
    ~test() { destructed++; }
    long value {};
    static inline size_t defaulted {};
    static inline size_t valued {};
    static inline size_t copied {};
    static inline size_t moved {};
    static inline size_t destructed {};
};

static int test_mirror() {
    infinite::array<long long> buffer(4096);
    expect(buffer.capacity() >= 4096);
    long long counter {};
    buffer.resize(4096);
    expect(buffer.size() == 4096);
    std::generate(buffer.begin(), buffer.end(), [&counter](){ return ++counter;} );
    auto data = buffer.begin();
    for(size_t l = 0; l < 4096; l++)
        if (expect_match(data[l + 4096], data[l])) {
            clog << "AT " << l << '\n';
            return 1;
        }
    return 0;
}

static int test_nointerfere() {
    infinite::array<long> buffer(4096);
    buffer.resize(buffer.capacity(), 102030405060708);
    buffer.erase(100);
    buffer.resize(buffer.capacity(), 203040506070809);
    infinite::array<double> dbl(2048);
    dbl.resize(dbl.capacity(), 1.01);
    dbl.erase(117);
    dbl.resize(dbl.capacity(), 0.02);
    buffer.erase(100);
    buffer.resize(buffer.capacity(), 304050607080900);
    auto size = buffer.size();
    for(size_t l = 0; l < size; l++) {
        const long expected = l < size - 200 ? 102030405060708 : l < size - 100 ? 203040506070809 : 304050607080900;
        if (expect_match(buffer[l], expected)) {
            clog << "AT " << l << '\n';
            return 1;
        }
    }
    size = dbl.size();
    for(size_t l = 0; l < size; l++) {
        const double expected = l < size - 117 ? 1.01 : 0.02;
        if (expect_match(dbl[l], expected) ) {
            clog << "AT " << l << '\n';
            return 1;
        }
    }
    return 0;
}


static int test_process() {
    infinite::array<unsigned long long> buffer(4096);
    unsigned long long counter {};
    while(counter < buffer.capacity() * 4) {
        array<unsigned long long, 192> data;
        if (buffer.size() >= 512) {
            buffer.erase(512);
        }
        std::generate(data.begin(), data.end(), [&counter](){ return counter++;} );
        buffer.append(data);
    }
    unsigned long long expected = counter - buffer.size();
    for(auto i = buffer.begin(); i != buffer.end(); i++) {
        if(expect_match(*i, expected++)) {
            clog << "AT " << (i - buffer.begin()) << '\n';
            return 1;
        }
    }
    for(auto i = buffer.rbegin(); i != buffer.rend(); i++) {
        if(expect_match(*i, --counter)) {
            clog << "AT " << (buffer.rend() - i) << '\n';
            return 1;
        }
    }
    return 0;
}

static bool test_construct() {{
    infinite::array<test> tests{1024};
    tests.emplace_back(12);
    tests.push_back(11);
    tests.append(tests.begin(), tests.begin()+2);
    tests.resize(8);
    tests.append({7,8,9,10}); }
    return expect_match(test::destructed, (test::defaulted + test::valued + test::copied + test::moved));
}

int main() {
	int fail_count =
	test_mirror() +
	test_construct() +
	test_process() +
	test_nointerfere();
	// TODO test struct with odd alignment
	return fail_count;
}
