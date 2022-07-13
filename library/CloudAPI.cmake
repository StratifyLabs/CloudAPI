
if(NOT DEFINED API_IS_SDK)
	include(InetAPI)
	include(JsonAPI)
	cmsdk_include_target(CloudAPI "${API_CONFIG_LIST}")
endif()
