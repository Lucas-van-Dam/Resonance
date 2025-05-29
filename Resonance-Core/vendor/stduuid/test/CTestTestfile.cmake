# CMake generated Testfile for 
# Source directory: C:/Projects/Cuttlefish/Resonance/Resonance-Core/vendor/stduuid/test
# Build directory: C:/Projects/Cuttlefish/Resonance/Resonance-Core/vendor/stduuid/test
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(test_stduuid "C:/Projects/Cuttlefish/Resonance/Resonance-Core/vendor/stduuid/test/Debug/test_stduuid.exe" "-r compact")
  set_tests_properties(test_stduuid PROPERTIES  FAIL_REGULAR_EXPRESSION "Failed \\d+ test cases" PASS_REGULAR_EXPRESSION "Passed all.*" TIMEOUT "120" _BACKTRACE_TRIPLES "C:/Projects/Cuttlefish/Resonance/Resonance-Core/vendor/stduuid/test/CMakeLists.txt;22;add_test;C:/Projects/Cuttlefish/Resonance/Resonance-Core/vendor/stduuid/test/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(test_stduuid "C:/Projects/Cuttlefish/Resonance/Resonance-Core/vendor/stduuid/test/Release/test_stduuid.exe" "-r compact")
  set_tests_properties(test_stduuid PROPERTIES  FAIL_REGULAR_EXPRESSION "Failed \\d+ test cases" PASS_REGULAR_EXPRESSION "Passed all.*" TIMEOUT "120" _BACKTRACE_TRIPLES "C:/Projects/Cuttlefish/Resonance/Resonance-Core/vendor/stduuid/test/CMakeLists.txt;22;add_test;C:/Projects/Cuttlefish/Resonance/Resonance-Core/vendor/stduuid/test/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(test_stduuid "C:/Projects/Cuttlefish/Resonance/Resonance-Core/vendor/stduuid/test/MinSizeRel/test_stduuid.exe" "-r compact")
  set_tests_properties(test_stduuid PROPERTIES  FAIL_REGULAR_EXPRESSION "Failed \\d+ test cases" PASS_REGULAR_EXPRESSION "Passed all.*" TIMEOUT "120" _BACKTRACE_TRIPLES "C:/Projects/Cuttlefish/Resonance/Resonance-Core/vendor/stduuid/test/CMakeLists.txt;22;add_test;C:/Projects/Cuttlefish/Resonance/Resonance-Core/vendor/stduuid/test/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(test_stduuid "C:/Projects/Cuttlefish/Resonance/Resonance-Core/vendor/stduuid/test/RelWithDebInfo/test_stduuid.exe" "-r compact")
  set_tests_properties(test_stduuid PROPERTIES  FAIL_REGULAR_EXPRESSION "Failed \\d+ test cases" PASS_REGULAR_EXPRESSION "Passed all.*" TIMEOUT "120" _BACKTRACE_TRIPLES "C:/Projects/Cuttlefish/Resonance/Resonance-Core/vendor/stduuid/test/CMakeLists.txt;22;add_test;C:/Projects/Cuttlefish/Resonance/Resonance-Core/vendor/stduuid/test/CMakeLists.txt;0;")
else()
  add_test(test_stduuid NOT_AVAILABLE)
endif()
