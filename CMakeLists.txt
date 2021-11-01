cmake_minimum_required (VERSION 3.12)

project(CloudAPI CXX)

include(CTest)

add_subdirectory(library library)
option(CLOUD_API_IS_TEST "Enable test builds for CloudAPI" OFF)
if(CLOUD_API_IS_TEST)
	add_subdirectory(tests tests)
endif()
