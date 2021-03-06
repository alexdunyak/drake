project(drake)
cmake_minimum_required(VERSION 3.5)

cmake_policy(SET CMP0025 NEW)
cmake_policy(SET CMP0042 NEW)

include(../cmake/config.cmake)

drake_setup_platform()

# Create a cache variable that contains the
# max number of concurent link jobs that can be run
# in a Ninja build, if the value is 0 then let Ninja
# use as many as it wants.
set(CMAKE_NINJA_LINK_POOL_SIZE 0 CACHE STRING
  "Number of concurent link jobs that can be run with Ninja build tool")
# if the link pool is non-zero then set the property
# on all the targets in drake
if(CMAKE_NINJA_LINK_POOL_SIZE GREATER 0)
  # Ninja: Configure a job pool to limit simultaneous linking.
  set_property(GLOBAL PROPERTY JOB_POOLS link_pool=${CMAKE_NINJA_LINK_POOL_SIZE})
  # Ninja: Assign all linking to our link job pool.
  set(CMAKE_JOB_POOL_LINK link_pool)
endif()

# options
option(BUILD_SHARED_LIBS "Build Drake with shared libraries." ON)
option(LONG_RUNNING_TESTS "some tests should be run nightly for coverage, but are too slow for CI" OFF)
option(RANDOMIZE_MATLAB_TESTS "every MATLAB unit is started with a new random seed" OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# prepend module path so we can override FindMatlab.cmake
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
if(WITH_PYTHON_3)
  set(Python_ADDITIONAL_VERSIONS 3.5)
  find_package(PythonInterp 3)
else()
  find_package(PythonInterp)
endif()

# set up pods
set(POD_NAME drake)
include(cmake/pods.cmake)

function(drake_install_headers)
  file(RELATIVE_PATH rel_path ${PROJECT_SOURCE_DIR}/ ${CMAKE_CURRENT_SOURCE_DIR})
  pods_install_headers(${ARGV} DESTINATION drake/${rel_path})
endfunction()

# set up automatic export header generation
# export headers will be installed to build/include/drake, but are created in ${PROJECT_BINARY_DIR}/exports/drake
# This means that export headers can be included as drake/bla_export.h
include(GenerateExportHeader)
function(add_library_with_exports)
  set(options STATIC)
  set(oneValueArgs LIB_NAME)
  set(multiValueArgs SOURCE_FILES)
  cmake_parse_arguments(parsed_args "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(parsed_args_STATIC)
    add_library(${parsed_args_LIB_NAME} STATIC ${parsed_args_SOURCE_FILES})
    # Add a public preprocessor macro definition for LIB_NAME_STATIC_DEFINE,
    # which suppresses platform-specific shared library export declarations
    # in this target and all targets that depend on it.
    string(TOUPPER ${parsed_args_LIB_NAME} capitalized_lib_name)
    set(static_define ${capitalized_lib_name}_STATIC_DEFINE)
    target_compile_definitions(${parsed_args_LIB_NAME} PUBLIC ${static_define})
  else()
    add_library(${parsed_args_LIB_NAME} SHARED ${parsed_args_SOURCE_FILES})
  endif()

  set(exports_abs_path ${PROJECT_BINARY_DIR}/exports/drake/${parsed_args_LIB_NAME}_export.h)
  generate_export_header(${parsed_args_LIB_NAME} EXPORT_FILE_NAME ${exports_abs_path})
  install(FILES ${exports_abs_path} DESTINATION include/drake/)
endfunction()
include_directories(BEFORE ${PROJECT_BINARY_DIR}/exports/)
include_directories(BEFORE ${PROJECT_SOURCE_DIR}/..)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=all -Werror=ignored-qualifiers")
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # TODO(#2852) Turn on shadow checking for g++ once we use a version that fixes
    # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=57709
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=shadow")
  endif()
  set(CXX_FLAGS_NO_ERROR_SHADOW "-Wno-error=shadow -Wno-shadow")
  set(CXX_FLAGS_NO_SIGN_COMPARE "-Wno-sign-compare")
elseif(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4251")
  # disable warning C4251: e.g.'RigidBody::linkname' : class 'std::basic_string<_Elem,_Traits,_Ax>' needs to have dll-interface to be used by clients of class 'RigidBody'
  # followed by template linking errors. After reading, e.g.:
  #   https://connect.microsoft.com/VisualStudio/feedback/details/696593/vc-10-vs-2010-basic-string-exports
  # I think we're not being sufficiently careful with our library interfaces (on other platforms as well) - Russ

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4503")
  # disable C4503: 'identifier' : decorated name length exceeded, name was truncated
  # these occur due to AutoDiffScalar inputs to DrakeJoint methods, which results in very long type names
  # From https://msdn.microsoft.com/en-us/library/074af4b6.aspx:
  # It is possible to ship an application that generates C4503, but if you get link time errors on a truncated symbol,
  # it will be more difficult to determine the type of the symbol in the error. Debugging will also be more difficult;
  # the debugger will also have difficultly mapping symbol name to type name. The correctness of the program, however,
  # is unaffected by the truncated name.

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4522")
  # disable C4522: 'class' : multiple assignment operators specified
  # The class has multiple assignment operators of a single type. This warning is informational;
  # the constructors are callable in your program.

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4996") # disable sprintf security warning
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj") # after receiving RigidBodyTree.cpp : fatal error C1128: number of sections exceeded object file format limit: compile with /bigobj

  add_definitions(-D_USE_MATH_DEFINES) # to get math constants on MSVC (see https://msdn.microsoft.com/en-us/library/4hwaceh6.aspx)

  set(CXX_FLAGS_NO_ERROR_SHADOW "/wd4456 /wd4457")
  set(CXX_FLAGS_NO_SIGN_COMPARE "/wd4018")
endif()

# similar to the export headers, any other generated headers will be installed to build/include/drake, but will be created in ${PROJECT_BINARY_DIR}/exports/drake
# These headers can be included as drake/bla.h
set(generated_dir ${PROJECT_BINARY_DIR}/generated)
set(drake_path_header ${generated_dir}/drake/Path.h)
configure_file(core/Path.h.in ${drake_path_header})
install(FILES ${drake_path_header} DESTINATION include/drake/)
include_directories(BEFORE ${generated_dir})

# set up matlab build
include(cmake/mex.cmake)
if(MATLAB_EXECUTABLE) # Set by drake_setup_platform
  if(DISABLE_MATLAB)
    message(STATUS "MATLAB is disabled because the CMake option DISABLE_MATLAB is set to ON.")
  else()
    # Search for MATLAB and configure various MEX options.
    # If MATLAB is found, MATLAB_FOUND is set.
    mex_setup()

    # Use find_package to find "all of matlab" along with what is needed to use
    # matlab_add_unit_test
    find_package(Matlab COMPONENTS MAIN_PROGRAM REQUIRED)
  endif()
endif()

# add ctest for a MATLAB string
# add_matlab_test(NAME <testname> COMMAND <matlab_string_to_eval>
#   [CONFIGURATIONS <config>...]
#   [WORKING_DIRECTORY <dir>]
#   [PROPERTIES <prop1> <value1> ...]
#   [REQUIRES <package1> <package2> ...])
# note: default working directory is CMAKE_CURRENT_SOURCE_DIR
# note: properties are set internally using set_tests_properties (<testname> PROPERTIES <prop1> <value1> ...)
# note: packages in REQUIRES must write a file addpath_<package>.m to CMAKE_INSTALL_PREFIX/matlab
function(add_matlab_test)
  # no matlab no tests
  if(NOT MATLAB_FOUND)
    return()
  endif()
  set(oneValueArgs NAME COMMAND WORKING_DIRECTORY)
  set(multiValueArgs PROPERTIES REQUIRES)
  cmake_parse_arguments(parsed_args "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(parsed_args_REQUIRES)
    foreach(require ${parsed_args_REQUIRES})
      if(NOT EXISTS "${CMAKE_INSTALL_PREFIX}/matlab/addpath_${require}.m")
        message(STATUS "Not running ${parsed_args_NAME} because ${require} was not installed.")
        return()
      endif()
    endforeach()
  endif()

  string(REPLACE ' '' parsed_args_COMMAND ${parsed_args_COMMAND}) # turn ' into '' so we can eval it in MATLAB
  if(NOT parsed_args_WORKING_DIRECTORY)
    set(parsed_args_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  set(exit_status "~strncmp(ex.identifier,'Drake:MissingDependency',23)")  # missing dependency => pass
  set(additional_paths "${CMAKE_INSTALL_PREFIX}/matlab;${parsed_args_WORKING_DIRECTORY};")
  set(cmd_to_run_before_test "addpath_drake; global g_disable_visualizers; g_disable_visualizers=true;")
  set(custom_Matlab_test_command "try, eval('${parsed_args_COMMAND}'); catch ex, disp(getReport(ex,'extended')); disp(' '); force_close_system; exit(${exit_status}); end; force_close_system; exit(0)")

  if(RANDOMIZE_MATLAB_TESTS)
    set(custom_Matlab_test_command "rng('shuffle'); rng_state=rng; disp(sprintf('To reproduce this test use rng(%d,''%s'')',rng_state.Seed,rng_state.Type)); disp(' '); ${custom_Matlab_test_command}")
  endif()

  matlab_add_unit_test(NAME ${parsed_args_NAME}
    ADDITIONAL_PATH ${additional_paths}
    UNITTEST_PRECOMMAND ${cmd_to_run_before_test}
    CUSTOM_TEST_COMMAND \"${custom_Matlab_test_command}\"
    WORKING_DIRECTORY ${parsed_args_WORKING_DIRECTORY}
    TIMEOUT -1) # use an infinite timeout and rely on add_test ctest timeout

  if(parsed_args_PROPERTIES)
    set_tests_properties(${parsed_args_NAME} PROPERTIES ${parsed_args_PROPERTIES})
  endif()
endfunction()

enable_testing()
include(CTest)
set(GTEST_ROOT ${CMAKE_INSTALL_PREFIX})
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGTEST_DONT_DEFINE_FAIL=1")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGTEST_DONT_DEFINE_SUCCEED=1")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGTEST_DONT_DEFINE_TEST=1")
if(WIN32)
  # The following compiler flag is needed to support parameterized Google tests on Windows.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGTEST_LINKED_AS_SHARED_LIBRARY=1")
endif()

set(drake_jar_javafiles util/Transform.java)
set(drake_jar_requires)

include(cmake/procman.cmake)  # helper script for writing procman files

# set up pseudo-target for threads
find_package(Threads)
add_library(threads INTERFACE)
target_link_libraries(threads INTERFACE ${CMAKE_THREAD_LIBS_INIT})

# set up and build lcm types
find_package(lcm)
if(lcm_FOUND)
  add_subdirectory(lcmtypes)
  include_directories(${drake_BINARY_DIR} ${drake_BINARY_DIR}/lcmtypes)

  list(APPEND drake_jar_javafiles
    util/MyLCMTypeDatabase.java
    util/MessageMonitor.java
    util/CoordinateFrameData.java
    util/LCMCoder.java)
  list(APPEND drake_jar_requires lcm-java lcmtypes_drake-java)

  # add lcmgl
  # annoyingly, libbot does not define a classpath variable. there might be a cleaner way to do this
  execute_process(COMMAND pkg-config --variable=prefix bot2-lcmgl-client OUTPUT_VARIABLE LCMGL_BUILD_DIR)
  if(NOT LCMGL_BUILD_DIR)
    message(WARNING "\n\n----- WARNING: bot2-lcmgl not found")
  else()
    string(STRIP ${LCMGL_BUILD_DIR} LCMGL_BUILD_DIR)
    find_jar(LCMGL_JAR_FILE bot2-lcmgl PATHS "${LCMGL_BUILD_DIR}/share/java/")
    if(NOT LCMGL_JAR_FILE)
      message(FATAL_ERROR "\n\n----- ERROR: bot2-lcmgl not found")
    else()
      message(STATUS "Found bot2-lcmgl")
    endif()
    set(CMAKE_JAVA_INCLUDE_PATH ${CMAKE_JAVA_INCLUDE_PATH}:${LCMGL_JAR_FILE})
    list(APPEND drake_jar_javafiles util/BotLCMGLClient.java)
  endif()
endif()

add_jar(drake SOURCES ${drake_jar_javafiles} INCLUDE_JARS ${drake_jar_requires})
install_jar(drake share/java)
pods_install_pkg_config_file(drake-java
  CLASSPATH drake
  REQUIRES ${drake_jar_requires}
  VERSION 0.0.1)

pods_find_pkg_config(eigen3 3.2.92)

if(eigen3_FOUND)
  pods_use_pkg_config_includes(eigen3)
else()
  # then use the traditional approach to finding Eigen3
  find_package(Eigen3 3.2.92)
  if(EIGEN3_FOUND)
    set(eigen3_FOUND 1)
    include_directories(${EIGEN3_INCLUDE_DIR})
  else()
    message(FATAL_ERROR "Could not find eigen, which is a required depedency")
  endif()
endif()

include_directories(thirdParty/bsd/spruce/include)

add_subdirectory(common)
add_subdirectory(math)
add_subdirectory(core)
add_subdirectory(util)
add_subdirectory(solvers)
add_subdirectory(systems)
add_subdirectory(thirdParty/bsd/GetFullPath)
add_subdirectory(thirdParty/bsd/spruce)
add_subdirectory(thirdParty/zlib/tinyxml2)
add_subdirectory(examples)
add_subdirectory(bindings)

# setup MATLAB pods-compliance
include(cmake/matlab_pods.cmake)
pods_configure_matlab_paths()

message(STATUS "Writing path utilities to ${CMAKE_INSTALL_PREFIX}/matlab")
file(WRITE ${CMAKE_INSTALL_PREFIX}/matlab/addpath_drake.m
  "function addpath_drake()\n"
  "  addpath(fullfile('${CMAKE_INSTALL_PREFIX}', 'matlab'));\n"
  "  wd = cd('${CMAKE_CURRENT_SOURCE_DIR}');\n"
  "  addpath_drake();\n"
  "  cd(wd);\n"
  "end\n"
  "\n")

file(WRITE ${CMAKE_INSTALL_PREFIX}/matlab/rmpath_drake.m
  "function rmpath_drake()\n"
  "  wd = cd('${CMAKE_CURRENT_SOURCE_DIR}');\n"
  "  rmpath_drake();\n"
  "  cd(wd);\n")

file(WRITE ${CMAKE_INSTALL_PREFIX}/matlab/get_drake_binary_dir.m
  "function [binary_dir] = get_drake_binary_dir()\n"
  "  binary_dir = '${PROJECT_BINARY_DIR}';\n"
  "end\n"
  "\n")

file(WRITE ${CMAKE_INSTALL_PREFIX}/matlab/get_drake_install_dir.m
  "function [install_dir] = get_drake_install_dir()\n"
  "  install_dir = '${CMAKE_INSTALL_PREFIX}';\n"
  "end\n"
  "\n")

find_program(avl avl PATHS ${CMAKE_INSTALL_DIR}/bin)
find_program(xfoil xfoil PATHS ${CMAKE_INSTALL_DIR}/bin)
find_program(ffmpeg ffmpeg)

if(APPLE)
  set(DYLD_LIBRARY_PATH "$ENV{DYLD_LIBRARY_PATH}" CACHE STRING "Environment variable used to launch processes from Matlab")
  set(DYLD_FRAMEWORK_PATH "$ENV{DYLD_FRAMEWORK_PATH}" CACHE STRING "Environment variable used to launch processes from Matlab")
  mark_as_advanced(DYLD_LIBRARY_PATH)
  mark_as_advanced(DYLD_FRAMEWORK_PATH)
else()
  set(LD_LIBRARY_PATH "$ENV{LD_LIBRARY_PATH}" CACHE STRING "Environment variable used to launch processes from Matlab")
  mark_as_advanced(LD_LIBRARY_PATH)
endif()

if(MATLAB_FOUND)
  add_matlab_test(NAME "RigidBodyManipulatorMemoryTest"
    COMMAND "r = RigidBodyManipulator('Acrobot.urdf'), megaclear"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/examples/Acrobot
    PROPERTIES TIMEOUT 300)
endif()

option(ENABLE_DOCUMENTATION "Enable build target for Doxygen and Sphinx documentation." ON)

if(ENABLE_DOCUMENTATION)
  add_subdirectory(doc)
endif()

if(IS_DIRECTORY doc/textbook)
  add_subdirectory(doc/textbook)
endif()

add_subdirectory(regtests)
