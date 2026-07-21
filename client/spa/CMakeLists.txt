add_library(AmneziaSpa STATIC)

if(NOT TARGET OpenSSL::Crypto)
    find_package(OpenSSL REQUIRED)
endif()

add_library(Amnezia::Spa ALIAS AmneziaSpa)

target_sources(AmneziaSpa
    PRIVATE
        src/gmSpaCodec.cpp
        src/spaConfig.cpp
        src/spaClient.cpp
        src/spaCrypto.cpp
        src/spaDeviceIdentity.cpp
        src/spaUiController.cpp
    PUBLIC
        include/spa/gmSpaCodec.h
        include/spa/spaConfig.h
        include/spa/spaClient.h
        include/spa/spaCodec.h
        include/spa/spaDeviceIdentity.h
        include/spa/spaTypes.h
        include/spa/spaUiController.h
        src/spaCrypto.h
)

qt_add_resources(AmneziaSpa spa_config
    PREFIX "/spa"
    FILES
        resources/spa-config.json
)

target_include_directories(AmneziaSpa
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(AmneziaSpa
    PUBLIC
        Qt6::Core
        Qt6::Network
        OpenSSL::Crypto
)

target_compile_definitions(AmneziaSpa
    PRIVATE
        $<$<CONFIG:Debug>:SPA_DEBUG_LOGGING>
)

set_target_properties(AmneziaSpa PROPERTIES
    AUTOMOC ON
    FOLDER "Libraries"
)
