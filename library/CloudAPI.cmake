
if(NOT DEFINED API_IS_SDK)
	include(InetAPI)
	include(JsonAPI)
	sos_sdk_include_target(CloudAPI "${API_CONFIG_LIST}")
endif()
