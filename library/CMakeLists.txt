
set(LIBRARIES JsonAPI InetAPI ThreadAPI)

api2_add_library(
  NAME ${PROJECT_NAME}
  DEPENDENCIES JsonAPI InetAPI ThreadAPI)

