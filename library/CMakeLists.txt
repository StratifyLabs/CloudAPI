
set(LIBRARIES JsonAPI InetAPI ThreadAPI)

cmsdk2_check_version(
  NAME InetAPI
  VERSION ${FsAPI_VERSION}
  MINIMUM_VERSION 1.5.2)

cmsdk2_check_version(
  NAME JsonAPI
  VERSION ${FsAPI_VERSION}
  MINIMUM_VERSION 1.5.1)

api2_add_library(
  NAME ${PROJECT_NAME}
  DEPENDENCIES JsonAPI InetAPI ThreadAPI)

