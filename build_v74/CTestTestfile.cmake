# CMake generated Testfile for 
# Source directory: D:/Desktop/通达信中枢安装包/chan_tdx_plugin
# Build directory: D:/Desktop/通达信中枢安装包/chan_tdx_plugin/build_v74
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(ChanCoreTests "D:/Desktop/通达信中枢安装包/chan_tdx_plugin/build_v74/bin/Debug/test_chan_core.exe")
  set_tests_properties(ChanCoreTests PROPERTIES  _BACKTRACE_TRIPLES "D:/Desktop/通达信中枢安装包/chan_tdx_plugin/CMakeLists.txt;104;add_test;D:/Desktop/通达信中枢安装包/chan_tdx_plugin/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(ChanCoreTests "D:/Desktop/通达信中枢安装包/chan_tdx_plugin/build_v74/bin/Release/test_chan_core.exe")
  set_tests_properties(ChanCoreTests PROPERTIES  _BACKTRACE_TRIPLES "D:/Desktop/通达信中枢安装包/chan_tdx_plugin/CMakeLists.txt;104;add_test;D:/Desktop/通达信中枢安装包/chan_tdx_plugin/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(ChanCoreTests "D:/Desktop/通达信中枢安装包/chan_tdx_plugin/build_v74/bin/MinSizeRel/test_chan_core.exe")
  set_tests_properties(ChanCoreTests PROPERTIES  _BACKTRACE_TRIPLES "D:/Desktop/通达信中枢安装包/chan_tdx_plugin/CMakeLists.txt;104;add_test;D:/Desktop/通达信中枢安装包/chan_tdx_plugin/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(ChanCoreTests "D:/Desktop/通达信中枢安装包/chan_tdx_plugin/build_v74/bin/RelWithDebInfo/test_chan_core.exe")
  set_tests_properties(ChanCoreTests PROPERTIES  _BACKTRACE_TRIPLES "D:/Desktop/通达信中枢安装包/chan_tdx_plugin/CMakeLists.txt;104;add_test;D:/Desktop/通达信中枢安装包/chan_tdx_plugin/CMakeLists.txt;0;")
else()
  add_test(ChanCoreTests NOT_AVAILABLE)
endif()
