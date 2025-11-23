cmake_minimum_required(VERSION 3.12...3.31)
project(PRmanager CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DownloadUserver)

find_package(
  userver
  COMPONENTS core #
             postgresql
  QUIET)

if(NOT userver_FOUND)
  set(USERVER_DISABLE_USERFAULTFD
      ON
      CACHE BOOL "" FORCE)
  download_userver(TRY_DIR third_party/userver)
endif()

userver_setup_environment()

# Common sources
include_directories(src)

file(GLOB_RECURSE SOURCES "src/handlers/*.cpp" "src/models/*.cpp")

add_library(${PROJECT_NAME}_objs OBJECT ${SOURCES})
target_link_libraries(${PROJECT_NAME}_objs PUBLIC userver::core
                                                  userver::postgresql)

# The Service
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_objs)

file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/unittests/*.cpp")
if(TEST_SOURCES)
  add_executable(${PROJECT_NAME}_unittests ${TEST_SOURCES})
  target_link_libraries(${PROJECT_NAME}_unittests PRIVATE ${PROJECT_NAME}_objs
                                                          userver::utest)
  add_google_tests(${PROJECT_NAME}_unittests)
endif()

# Benchmarks - disabled for now (placeholder benchmarks reference non-existent
# functions) add_executable(${PROJECT_NAME}_benchmark
# src/greeting_benchmark.cpp) target_link_libraries(${PROJECT_NAME}_benchmark
# PRIVATE ${PROJECT_NAME}_objs userver::ubench)
# add_google_benchmark_tests(${PROJECT_NAME}_benchmark)

# Functional testing
userver_testsuite_add_simple()

# Install
include(GNUInstallDirs)

if(DEFINED ENV{PREFIX})
  message(STATUS "Set install prefix: $ENV{PREFIX}")
  file(TO_CMAKE_PATH "$ENV{PREFIX}" PREFIX_PATH)
  set(CMAKE_INSTALL_PREFIX "${PREFIX_PATH}")
endif()

file(GLOB CONFIGS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.yaml
     ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.json)

install(
  TARGETS ${PROJECT_NAME}
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT ${PROJECT_NAME})
install(
  FILES ${CONFIGS_FILES}
  DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/${PROJECT_NAME}
  COMPONENT ${PROJECT_NAME})
