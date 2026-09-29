// Test runner for the des test suites. Run with --help for usage.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

#include "des_test.hpp"

namespace des_test
{
	namespace
	{
		std::mutex mtx;
		std::vector<std::string> failures;
		std::vector<std::string> notes;
		bool golden = false;
	}

	std::vector<test_case>& registry()
	{
		static std::vector<test_case> cases;
		return cases;
	}

	void fail(const char* file, int line, const std::string& message)
	{
		std::lock_guard<std::mutex> lock(mtx);
		failures.push_back(std::string(file) + ":" + std::to_string(line) + ": " + message);
	}

	void note(const std::string& message)
	{
		std::lock_guard<std::mutex> lock(mtx);
		notes.push_back(message);
	}

	bool update_golden()
	{
		return golden;
	}
}

namespace
{
	void usage(const char* argv0)
	{
		std::printf(
			"usage: %s [options] [filter...]\n"
			"\n"
			"filters (a test runs if it matches any include and no exclude):\n"
			"  [tag]        include the tests with this tag, e.g. [unit]\n"
			"  ~[tag]       exclude the tests with this tag\n"
			"  text         include the tests whose name contains text\n"
			"\n"
			"options:\n"
			"  --list            list the selected tests and exit\n"
			"  -v, --verbose     print the notes of passing tests too\n"
			"  --update-golden   regression tests write their golden files instead of comparing\n"
			"  -h, --help        show this help\n",
			argv0);
	}

	bool has_tag(const des_test::test_case& t, const std::string& tag)
	{
		return t.tags.find(tag) != std::string::npos;
	}
}

int main(int argc, char** argv)
{
	std::vector<std::string> include_tags, exclude_tags, names;
	bool list = false, verbose = false;
	for(int i = 1; i < argc; i++)
	{
		std::string a = argv[i];
		if(a == "--list") list = true;
		else if(a == "-v" || a == "--verbose") verbose = true;
		else if(a == "--update-golden") des_test::golden = true;
		else if(a == "-h" || a == "--help") { usage(argv[0]); return 0; }
		else if(a.size() > 2 && a[0] == '~' && a[1] == '[') exclude_tags.push_back(a.substr(1));
		else if(!a.empty() && a[0] == '[') include_tags.push_back(a);
		else if(!a.empty() && a[0] == '-') { std::fprintf(stderr, "unknown option %s\n", a.c_str()); usage(argv[0]); return 2; }
		else names.push_back(a);
	}

	std::vector<des_test::test_case> selected;
	for(const des_test::test_case& t : des_test::registry())
	{
		bool included = include_tags.empty() && names.empty();
		for(const std::string& tag : include_tags) included = included || has_tag(t, tag);
		for(const std::string& n : names) included = included || t.name.find(n) != std::string::npos;
		for(const std::string& tag : exclude_tags) included = included && !has_tag(t, tag);
		if(included) selected.push_back(t);
	}
	// Registration order across files is unspecified: sort for a stable output
	std::sort(selected.begin(), selected.end(), [](const des_test::test_case& a, const des_test::test_case& b)
	{
		int c = std::strcmp(a.file, b.file);
		return c != 0 ? c < 0 : a.line < b.line;
	});

	if(list)
	{
		for(const des_test::test_case& t : selected) std::printf("%-70s %s\n", t.name.c_str(), t.tags.c_str());
		std::printf("%zu test cases\n", selected.size());
		return 0;
	}
	if(selected.empty())
	{
		std::fprintf(stderr, "no test case matches the filters\n");
		return 2;
	}

	size_t passed = 0, failed = 0, skipped = 0, failed_checks = 0;
	auto start = std::chrono::steady_clock::now();
	for(const des_test::test_case& t : selected)
	{
		des_test::failures.clear();
		des_test::notes.clear();
		std::string skip_reason;
		bool skip = false;
		auto t0 = std::chrono::steady_clock::now();
		try
		{
			t.body();
		}
		catch(const des_test::abort_test&) {}
		catch(const des_test::skip_test& s) { skip = true; skip_reason = s.reason; }
		catch(const std::exception& e) { des_test::fail(t.file, t.line, std::string("unexpected exception: ") + e.what()); }
		catch(...) { des_test::fail(t.file, t.line, "unexpected exception"); }
		double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();

		if(!des_test::failures.empty())
		{
			++failed;
			failed_checks += des_test::failures.size();
			std::printf("[ FAIL ] %s (%.0f ms)\n", t.name.c_str(), ms);
			for(const std::string& f : des_test::failures) std::printf("         %s\n", f.c_str());
			for(const std::string& n : des_test::notes) std::printf("         note: %s\n", n.c_str());
		}
		else if(skip)
		{
			++skipped;
			std::printf("[ SKIP ] %s: %s\n", t.name.c_str(), skip_reason.c_str());
		}
		else
		{
			++passed;
			std::printf("[  OK  ] %s (%.0f ms)\n", t.name.c_str(), ms);
			if(verbose)
				for(const std::string& n : des_test::notes) std::printf("         note: %s\n", n.c_str());
		}
		std::fflush(stdout);
	}
	double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
	std::printf("\n%zu test cases: %zu passed, %zu failed (%zu checks), %zu skipped in %.2f s\n",
				selected.size(), passed, failed, failed_checks, skipped, secs);
	return failed == 0 ? 0 : 1;
}
