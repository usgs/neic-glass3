# cmake
This directory contains various cmake scripts that handle common functions like
code coverage, unit testing, documentation generation, etc.

These scripts include:
------
* base.cmake - a script that sets up basic complier settings and flags.
* build_exe.cmake - a script that builds an executable file for a submodule
(such as glass-app)
* build_lib.cmake - a script that builds a library file for a submodule
(such as util)
* cppcheck.cmake - a script that runs cpp error checks for a submodule prior to
the submodule build
* cpplint.cmake - a script that runs cpp linter (code style) checks on to the
submodule build
* documentation.cmake - a script that generates doxygen documentation for a
submodule
* Doxyfile.in - a doxygen configuration file that documentation.cmake configures
and uses during documentation generation
* FindLibuuid.cmake - a cmake script that searches for the libuuid library on
linux distributions
* install_exe.cmake - a script that installs an executable and configuration
file(s) for a submodule (such as glass-app)
* install_lib.cmake - a script that installs a library and header file (s) for a
submodule (such as util)
* internal_utils.cmake - a cmake macro that sets certain defaults for MSVC builds,
used by base.cmake
* package_config.cmake.in - a cmake script configured and used by cmake define
cmake variables for a submodule so that it can be included by other cmake builds
* project_version.h.in - a header file configured and used by cmake to provide
the overall project version to the c++ code.
* test.cmake - a cmake script for running unit tests, generating coverage
information, and using generate_coverage.sh to create a coverage report
* version.cmake - a cmake script that defines the overall project version.

Other scripts:
------
* generate_coverage.sh - a bash script that generates a coverage report, and
filters out various system and external libraries, used by test.cmake
