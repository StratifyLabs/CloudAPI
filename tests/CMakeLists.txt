
set(DEPENDENCIES CloudAPI TestAPI FsAPI JsonAPI ThreadAPI CryptoAPI)

api_add_test_executable(${PROJECT_NAME} 65536 "${DEPENDENCIES}")




