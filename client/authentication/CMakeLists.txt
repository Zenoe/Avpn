add_library(CaelispectAuthentication STATIC)

add_library(Caelispect::Authentication ALIAS CaelispectAuthentication)

target_sources(CaelispectAuthentication
    PRIVATE
        src/authenticationController.cpp
        src/passwordCrypto.cpp
    PUBLIC
        include/authentication/authenticationController.h
    PRIVATE
        src/passwordCrypto.h
)

target_include_directories(CaelispectAuthentication
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
)

target_link_libraries(CaelispectAuthentication
    PUBLIC
        Qt6::Core
        Qt6::Network
        OpenSSL::Crypto
)

set_target_properties(CaelispectAuthentication PROPERTIES
    AUTOMOC ON
    FOLDER "Libraries"
)
