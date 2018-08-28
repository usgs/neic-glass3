C++ Coding Style
================

For C++ files (*.cpp and *.h), this project uses the Google C++ Style (https://google.github.io/styleguide/cppguide.html) with the
following modifications:

1. This project uses whitespace tab indentation instead of the google style
   space indentation.
2. This project does not use the google style copyright messages
3. This project does not use the google style header guard formatting
4. This project does not use the google style function size limit.
5. This project uses CamelCase for naming functions and variables.
6. This project identifies class members with a "m_" prefix.

The source code for this project should be annotated such that Doxygen can be
used to generate documentation files.

Furthermore, when feasible, the source code for this project should include
appropriate unit tests of functions, classes, and libraries. Coverage of
these tests should be checked with an appropriate code coverage tool.

### Tools

This project uses the following tools to enforce this code style, perform static
code analysis, and to run unit and coverage tests.  These tools can
automatically be run as part of the cmake build process.

* cpplint.py (https://github.com/google/styleguide/tree/gh-pages/cpplint)
  - Use this category-filters exclusion list when running cpplint: ```--filter=-whitespace/tab,-legal/copyright,-build/c++11,-build/header_guard,-readability/fn_size```

* GoogleCppStyleModifed.xml - A code style definition file for use with eclipse
  and other editors.
  - available in the `lib/cpplint` directory.

* cppcheck (http://cppcheck.sourceforge.net/)
  - Use the following options when running cppcheck: ```--enable=warning,performance,portability --language=c++ --std=c++11 --template="[{severity}][{id}] {message} {callstack} \(On {file}:{line}\)" --verbose --suppress=nullPointerRedundantCheck --error-exitcode=1```

* googletest (https://github.com/google/googletest)

* gcov (https://gcc.gnu.org/onlinedocs/gcc/Gcov.html)

* doxygen (http://www.stack.nl/~dimitri/doxygen/)

### Examples

``Example.h``

```C++
#ifndef EXAMPLE_H
#define EXAMPLE_H

#include <json.h>
#include <string>
#include <fstream>

namespace example {

class Example {
 public:
	Example();

	~Example();

	void clear();

	const std::string& getExampleString() const {
		return m_sExampleString;
	}

 protected:
	bool setExampleString(std::string newExample);

 private:
	std::string m_sExampleString;
};
}  // namespace example
#endif  // EXAMPLE_H
```

``Example.cpp``

```C++
#include <example.h>
#include <json.h>
#include <string>

namespace example {
  Example::Example() {
    clear();
  }

  Example::~Example(){
    clear();
  }

  void Example::clear(){
    m_sExampleString = "";
  }

  bool Example::setExampleString(std::string newExample) {
    m_sExampleString = newExample;
  }
}  // namespace example
```
