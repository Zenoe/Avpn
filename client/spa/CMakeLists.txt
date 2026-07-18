add_library(AmneziaSpa STATIC)

add_library(Amnezia::Spa ALIAS AmneziaSpa)

target_sources(AmneziaSpa
    PRIVATE
        src/spaClient.cpp
    PUBLIC
        include/spa/spaClient.h
        include/spa/spaCodec.h
        include/spa/spaTypes.h
)

target_include_directories(AmneziaSpa
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(AmneziaSpa
    PUBLIC
        Qt6::Core
        Qt6::Network
)

set_target_properties(AmneziaSpa PROPERTIES
    AUTOMOC ON
    FOLDER "Libraries"
)

