License
=======

Unless otherwise noted, This project is in the public domain in the United
States because it contains materials that originally came from the United
States Geological Survey, an agency of the United States Department of
Interior. For more information, see the official USGS copyright policy at
https://www2.usgs.gov/visual-id/credit_usgs.html#copyright

Additionally, we waive copyright and related rights in the work
worldwide through the CC0 1.0 Universal public domain dedication.


CC0 1.0 Universal Summary
-------------------------

This is a human-readable summary of the
[Legal Code (read the full text)][1].


### No Copyright

The person who associated a work with this deed has dedicated the work to
the public domain by waiving all of his or her rights to the work worldwide
under copyright law, including all related and neighboring rights, to the
extent allowed by law.

You can copy, modify, distribute and perform the work, even for commercial
purposes, all without asking permission.


### Other Information

In no way are the patent or trademark rights of any person affected by CC0,
nor are the rights that other persons may have in the work or in how the
work is used, such as publicity or privacy rights.

Unless expressly stated otherwise, the person who associated a work with
this deed makes no warranties about the work, and disclaims liability for
all uses of the work, to the fullest extent permitted by applicable law.
When using or citing the work, you should not imply endorsement by the
author or the affirmer.

* C++ Dependent libraries found in the `lib` directory are distributed under
the open source (or open source-like) licenses/agreements. Appropriate license
agreements for each library can be found in the `lib` directory.


Libraries used at runtime included in repository
------------------------------------------------

 - rapidJSON                    (https://github.com/miloyip/rapidjson)
     - Note that the problematic JSON license is avoided by excluding the
     jsonchecker which was the the only portion of rapidJSON under that license.
 - SuperEasyJSON                (https://sourceforge.net/projects/supereasyjson/)
 - spdlog                       (https://github.com/gabime/spdlog)
 - librdkafka                   (https://github.com/edenhill/librdkafka)

Libraries used at runtime downloaded by cmake
---------------------------------------------

 - earthquake-detection-formats (https://github.com/usgs/earthquake-detection-formats)     
 - hazdev-broker                (https://github.com/usgs/hazdev-broker)

Libraries and tools used to build/test/document package
-------------------------------------------------------

 - googletest                  (https://github.com/google/googletest)
 - cmake                       (http://www.cmake.org)
 - cpplint                     (https://github.com/google/styleguide)
 - cppcheck                    (http://cppcheck.sourceforge.net/)
 - doxygen                     (http://www.stack.nl/~dimitri/doxygen/)

[1]: https://creativecommons.org/publicdomain/zero/1.0/legalcode
