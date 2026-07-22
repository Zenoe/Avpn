set(CLIENT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/Modules;${CMAKE_MODULE_PATH}")

add_subdirectory(${CLIENT_ROOT_DIR}/3rd/SortFilterProxyModel ${CMAKE_BINARY_DIR}/3rd/SortFilterProxyModel)
set(LIBS ${LIBS} SortFilterProxyModel)

include(${CLIENT_ROOT_DIR}/3rd/qrcodegen/qrcodegen.cmake)

add_compile_definitions(_WINSOCKAPI_)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_WITH_QT6 ON)
add_subdirectory(${CLIENT_ROOT_DIR}/3rd/qtkeychain ${CMAKE_BINARY_DIR}/3rd/qtkeychain EXCLUDE_FROM_ALL)

set(LIBS ${LIBS} qt6keychain)

include_directories(
    ${CLIENT_ROOT_DIR}/3rd/qtkeychain/qtkeychain
    ${CMAKE_CURRENT_BINARY_DIR}/3rd/qtkeychain
)

find_package(OpenSSL REQUIRED)
list(APPEND LIBS OpenSSL::SSL OpenSSL::Crypto)
