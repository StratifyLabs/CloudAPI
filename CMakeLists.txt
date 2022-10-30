cmake_minimum_required (VERSION 3.12)
cmsdk2_minimum_required(2.1.2)

project(CloudAPI
	LANGUAGES CXX
	VERSION 1.3.2)

include(CTest)

add_subdirectory(library library)
option(CLOUD_API_IS_TEST "Enable test builds for CloudAPI" OFF)
if(CLOUD_API_IS_TEST)
	add_subdirectory(tests tests)
endif()
