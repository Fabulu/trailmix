// Minimal test framework — no external dependencies.
#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <vector>
#include <string>
#include <stdexcept>
#include <functional>
#include <iostream>

struct TestEntry {
    const char* name;
    std::function<void()> fn;
};

inline std::vector<TestEntry>& getTests() {
    static std::vector<TestEntry> tests;
    return tests;
}

#define TEST(name) \
    void name(); \
    static struct name##_reg { \
        name##_reg() { getTests().push_back({#name, name}); } \
    } name##_instance; \
    void name()

#define ASSERT(cond) \
    do { \
        if (!(cond)) { \
            throw std::runtime_error( \
                std::string("FAIL: ") + #cond + \
                " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while(0)

#define ASSERT_EQ(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (_a != _b) { \
            throw std::runtime_error( \
                std::string("FAIL: ") + #a + " == " + #b + \
                " (got " + std::to_string(_a) + " vs " + std::to_string(_b) + ")" + \
                " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while(0)

inline int runAllTests() {
    int passed = 0, failed = 0;
    for (auto& t : getTests()) {
        try {
            t.fn();
            std::cout << "  PASS: " << t.name << "\n";
            ++passed;
        } catch (const std::exception& ex) {
            std::cout << "  " << ex.what() << " in " << t.name << "\n";
            ++failed;
        }
    }
    std::cout << "\n" << passed << " passed, " << failed << " failed.\n";
    return failed ? 1 : 0;
}

#endif // TEST_FRAMEWORK_H
