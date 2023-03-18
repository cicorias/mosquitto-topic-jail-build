include(FetchContent)
FetchContent_Declare(mosquitto
    GIT_REPOSITORY      https://github.com/eclipse/mosquitto.git
    GIT_TAG             6f574f80ea151a328fd94789c2b336e0bd1fa115)
FetchContent_GetProperties(mosquitto)
if(NOT mosquittosrc_POPULATED)
    FetchContent_Populate(mosquitto)
    add_subdirectory(${mosquitto_SOURCE_DIR} ${mosquitto_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

