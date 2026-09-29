#ifndef DES_TEST_HPP
#define DES_TEST_HPP

/*
 * Minimal test harness for the des test suites, with no third-party dependency.
 *
 *   TEST_CASE("fifo releases in arrival order", "[unit][store]")
 *   {
 *       REQUIRE(store.size() == 2);             // stops the test case on failure
 *       CHECK_EQ(store.pop(0)->get_id(), id);   // records the failure and goes on
 *       CHECK_NEAR(mean, 5.0, 0.1);
 *       CHECK_THROWS_AS(std::invalid_argument, des::tag_registry::define("time"));
 *       NOTE("mean " << mean);                  // printed if the test fails, or with --verbose
 *   }
 *
 * Tags select the tiers run by the Makefile targets: [unit], [regression], [validation],
 * [threads]. The runner is test/harness/main.cpp (see --help).
 */

#include <cmath>
#include <exception>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace des_test
{
	struct test_case
	{
		std::string name;
		std::string tags;   ///< e.g. "[unit][store]"
		void (*body)();
		const char* file;
		int line;
	};

	/** All the registered test cases. */
	std::vector<test_case>& registry();
	/** Records a failed check of the running test case (thread-safe). */
	void fail(const char* file, int line, const std::string& message);
	/** Records a note shown if the running test case fails, or with --verbose (thread-safe). */
	void note(const std::string& message);
	/** True when the runner was started with --update-golden. */
	bool update_golden();

	/** Thrown by REQUIRE* to stop the running test case. */
	struct abort_test {};
	/** Thrown by SKIP to stop the running test case without failing it. */
	struct skip_test
	{
		std::string reason;
	};

	struct registrar
	{
		registrar(const char* name, const char* tags, void (*body)(), const char* file, int line)
		{
			registry().push_back(test_case{name, tags, body, file, line});
		}
	};

	template <typename T, typename = void>
	struct printable : std::false_type {};
	template <typename T>
	struct printable<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<const T&>())>> : std::true_type {};

	/** Text of a checked value, for failure messages. */
	template <typename T>
	std::string show(const T& v)
	{
		if constexpr (printable<T>::value)
		{
			std::ostringstream s;
			s.precision(17);
			s << v;
			return s.str();
		}
		else
		{
			return "<value>";
		}
	}
}

#define DES_TEST_CAT2(a, b) a##b
#define DES_TEST_CAT(a, b) DES_TEST_CAT2(a, b)
#define DES_TEST_CASE_IMPL(fn, name, tags) \
	static void fn(); \
	static des_test::registrar DES_TEST_CAT(fn, _registrar)(name, tags, &fn, __FILE__, __LINE__); \
	static void fn()

/** Defines a test case: TEST_CASE("name", "[tag1][tag2]") { ... } */
#define TEST_CASE(name, tags) DES_TEST_CASE_IMPL(DES_TEST_CAT(des_test_case_, __COUNTER__), name, tags)

/** Records a failure if the condition is false. */
#define CHECK(...) \
	do { if(!(__VA_ARGS__)) des_test::fail(__FILE__, __LINE__, "CHECK(" #__VA_ARGS__ ")"); } while(false)

/** Records a failure and stops the test case if the condition is false. */
#define REQUIRE(...) \
	do { if(!(__VA_ARGS__)) { des_test::fail(__FILE__, __LINE__, "REQUIRE(" #__VA_ARGS__ ")"); throw des_test::abort_test(); } } while(false)

/** Records a failure, showing both values, if a != b. */
#define CHECK_EQ(a, b) \
	do { \
		const auto& des_test_a_ = (a); \
		const auto& des_test_b_ = (b); \
		if(!(des_test_a_ == des_test_b_)) \
			des_test::fail(__FILE__, __LINE__, "CHECK_EQ(" #a ", " #b "): " + des_test::show(des_test_a_) + " != " + des_test::show(des_test_b_)); \
	} while(false)

/** Records a failure, showing both values, if |a - b| > tol. */
#define CHECK_NEAR(a, b, tol) \
	do { \
		const double des_test_a_ = (a), des_test_b_ = (b), des_test_t_ = (tol); \
		if(!(std::fabs(des_test_a_ - des_test_b_) <= des_test_t_)) \
			des_test::fail(__FILE__, __LINE__, "CHECK_NEAR(" #a ", " #b ", " #tol "): " + des_test::show(des_test_a_) + " vs " + des_test::show(des_test_b_)); \
	} while(false)

/** Records a failure if value lies outside the (low, high) pair interval. */
#define CHECK_IN(value, interval) \
	do { \
		const double des_test_v_ = (value); \
		const auto des_test_i_ = (interval); \
		if(!(des_test_i_.first <= des_test_v_ && des_test_v_ <= des_test_i_.second)) \
			des_test::fail(__FILE__, __LINE__, "CHECK_IN(" #value ", " #interval "): " + des_test::show(des_test_v_) + " outside [" + des_test::show(des_test_i_.first) + ", " + des_test::show(des_test_i_.second) + "]"); \
	} while(false)

/** Records a failure unless the statement throws an exception of the given type. */
#define CHECK_THROWS_AS(type, ...) \
	do { \
		bool des_test_thrown_ = false; \
		try { __VA_ARGS__; } catch(const type&) { des_test_thrown_ = true; } catch(...) {} \
		if(!des_test_thrown_) des_test::fail(__FILE__, __LINE__, "CHECK_THROWS_AS(" #type ", " #__VA_ARGS__ "): no " #type " thrown"); \
	} while(false)

/** Records a failure if the statement throws. */
#define CHECK_NOTHROW(...) \
	do { \
		try { __VA_ARGS__; } \
		catch(const std::exception& des_test_e_) { des_test::fail(__FILE__, __LINE__, std::string("CHECK_NOTHROW(" #__VA_ARGS__ "): threw ") + des_test_e_.what()); } \
		catch(...) { des_test::fail(__FILE__, __LINE__, "CHECK_NOTHROW(" #__VA_ARGS__ "): threw"); } \
	} while(false)

/** Adds a note to the running test case: NOTE("mean " << m << " theory " << t) */
#define NOTE(...) \
	do { std::ostringstream des_test_s_; des_test_s_ << __VA_ARGS__; des_test::note(des_test_s_.str()); } while(false)

/** Stops the running test case, reporting it as skipped. */
#define SKIP(...) \
	do { std::ostringstream des_test_s_; des_test_s_ << __VA_ARGS__; throw des_test::skip_test{des_test_s_.str()}; } while(false)

#endif
