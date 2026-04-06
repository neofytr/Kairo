#pragma once

#include <cstdio>
#include <cmath>
#include <string>
#include <vector>
#include <functional>

// minimal test framework — no external deps
// usage:
//   TEST(test_name) { ASSERT(condition); ASSERT_EQ(a, b); }
//   int main() { return run_all_tests(); }

namespace test {

struct TestCase {
    std::string name;
    std::function<void()> func;
};

inline std::vector<TestCase>& get_tests() {
    static std::vector<TestCase> tests;
    return tests;
}

inline int g_assertions = 0;
inline int g_failures = 0;
inline std::string g_current_test;

struct TestRegistrar {
    TestRegistrar(const char* name, std::function<void()> func) {
        get_tests().push_back({ name, std::move(func) });
    }
};

inline int run_all_tests() {
    int passed = 0;
    int failed = 0;

    printf("running %zu tests...\n\n", get_tests().size());

    for (auto& tc : get_tests()) {
        g_current_test = tc.name;
        g_assertions = 0;
        g_failures = 0;

        try {
            tc.func();
        } catch (const std::exception& e) {
            printf("  EXCEPTION: %s\n", e.what());
            g_failures++;
        } catch (...) {
            printf("  UNKNOWN EXCEPTION\n");
            g_failures++;
        }

        if (g_failures == 0) {
            printf("  PASS  %s (%d assertions)\n", tc.name.c_str(), g_assertions);
            passed++;
        } else {
            printf("  FAIL  %s (%d failures)\n", tc.name.c_str(), g_failures);
            failed++;
        }
    }

    printf("\n--- results: %d passed, %d failed ---\n", passed, failed);
    return failed > 0 ? 1 : 0;
}

} // namespace test

#define TEST(name) \
    static void test_func_##name(); \
    static test::TestRegistrar test_reg_##name(#name, test_func_##name); \
    static void test_func_##name()

#define ASSERT(cond) do { \
    test::g_assertions++; \
    if (!(cond)) { \
        printf("    ASSERT FAILED: %s (line %d)\n", #cond, __LINE__); \
        test::g_failures++; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    test::g_assertions++; \
    if ((a) != (b)) { \
        printf("    ASSERT_EQ FAILED: %s != %s (line %d)\n", #a, #b, __LINE__); \
        test::g_failures++; \
    } \
} while(0)

#define ASSERT_NEAR(a, b, eps) do { \
    test::g_assertions++; \
    if (std::abs((a) - (b)) > (eps)) { \
        printf("    ASSERT_NEAR FAILED: %s=%.6f != %s=%.6f (eps=%.6f, line %d)\n", \
               #a, (double)(a), #b, (double)(b), (double)(eps), __LINE__); \
        test::g_failures++; \
    } \
} while(0)

#define ASSERT_TRUE(cond) ASSERT(cond)
#define ASSERT_FALSE(cond) ASSERT(!(cond))
