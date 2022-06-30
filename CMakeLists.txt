cmake_minimum_required(VERSION 3.16)

project(devmem VERSION 0.0.1 LANGUAGES C)
set(CMAKE_C_STANDARD 11)

include(GNUInstallDirs)

find_package(PkgConfig REQUIRED)
pkg_search_module(LUA lua5.4 lua5.3 luajit lua5.2 lua5.1 lua REQUIRED)

message(STATUS "Found ${LUA_MODULE_NAME}")

if ("${LUA_MODULE_NAME}" STREQUAL "luajit")
  pkg_get_variable(LUA_VER ${LUA_MODULE_NAME} abiver)
else()
  # remove minor version (meta-oe pc file has no major_version)
  pkg_get_variable(LUA_VER ${LUA_MODULE_NAME} version)
  string(REGEX MATCH "^([0-9]+\\.[0-9]+)" LUA_VER ${LUA_VERSION})
endif()

if( "${LUA_VER}" STREQUAL "5.1" OR "${LUA_VER}" STREQUAL "5.2" )
  find_file(COMPAT53 compat-5.3.h PATH_SUFFIXES "lua${LUA_VER}")

  if(NOT COMPAT53)
    message(FATAL_ERROR "failed to find compat-5.3.h for lua${LUA_VER}, please install lua-compat53-dev")
  else()
    message(STATUS "lua-compat53 found")
  endif()
endif()

set(DEVMEM_SRCS src/devmem.c)
set(DEVMEM_INST_DIR ${CMAKE_INSTALL_PREFIX}/lib/lua/${LUA_VER}/)

add_library(devmem SHARED ${DEVMEM_SRCS})
target_compile_options(devmem PRIVATE -Wall -Wextra)
target_include_directories(devmem PRIVATE ${LUA_INCLUDE_DIRS})
target_link_libraries(devmem ${LUA_LIBRARIES})
set_target_properties(devmem PROPERTIES PREFIX "")
install(TARGETS devmem LIBRARY DESTINATION ${DEVMEM_INST_DIR})
