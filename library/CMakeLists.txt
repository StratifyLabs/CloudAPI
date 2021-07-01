
set(LIBRARIES JsonAPI InetAPI FsAPI ChronoAPI ThreadAPI)

api_add_api_library(${PROJECT_NAME} "${LIBRARIES}")

