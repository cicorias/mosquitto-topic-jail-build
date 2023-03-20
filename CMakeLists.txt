cmake_minimum_required(VERSION 3.25)
project(mosquitto_plugin VERSION 0.1.0)

set (PLUGIN_NAME mosquitto_topic_jail_all)

add_library(${PLUGIN_NAME} MODULE
	${PLUGIN_NAME}.c
)

if(APPLE)
	set(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "${CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS} -undefined dynamic_lookup")
endif()


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(addMosquitto)
target_link_libraries(${PLUGIN_NAME} PRIVATE mosquitto)

# new - from plugin source cmake.
target_include_directories(${PLUGIN_NAME} PRIVATE
	"${OPENSSL_INCLUDE_DIR}"
	"${STDBOOL_H_PATH}"
	"${STDINT_H_PATH}"
	"${mosquitto_SOURCE_DIR}"
	"${mosquitto_SOURCE_DIR}/include"
)

link_directories(${mosquitto_SOURCE_DIR})

set_target_properties(${PLUGIN_NAME} PROPERTIES
	PREFIX ""
	POSITION_INDEPENDENT_CODE 1
)
