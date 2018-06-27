# cmake
This directory contains various CMake scripts that handle common functions like
code coverage, unit testing, documentation generation, etc.

CMake Scripts:
------
* **base.cmake** - a script that sets up basic complier settings and flags.
* **build_exe.cmake** - a script that builds an executable file for a submodule
(such as glass-app)
* **build_lib.cmake** - a script that builds a library file for a submodule
(such as util)
* **cppcheck.cmake** - a script that runs cpp error checks for a submodule prior to
the submodule build
* **cpplint.cmake** - a script that runs cpp linter (code style) checks on to the
submodule build
* **documentation.cmake** - a script that generates DOxygen documentation for a
submodule
* **FindLibuuid.cmake** - a CMake script that searches for the libuuid library on
linux distributions, used by uuid.cmake
* **install_exe.cmake** - a script that installs an executable and configuration
file(s) for a submodule (such as glass-app)
* **install_lib.cmake** - a script that installs a library and header file (s) for a
submodule (such as util)
* **test.cmake** - a CMake script for running unit tests, generating coverage
information, and using generate_coverage.sh to create a coverage report
* **uuid.cmake** - a CMake script that handles uuid on linux distributions
* **version.cmake** - a CMake script that defines the overall project version.

CMake configured files:
------
* **Doxyfile.in** - a DOxygen configuration file that documentation.cmake configures
and uses during documentation generation
* **project_version.h.in** - a C++ header file configured and used by CMake to provide
the overall project version to the c++ code.
* **package_config.cmake.in** - a CMake script configured and used by CMake to define
CMake variables for a submodule so that it can be included by other CMake builds

Other scripts:
------
* **generate_coverage.sh** - a bash script that generates a coverage report, and
filters out various system and external libraries, used by test.cmake
