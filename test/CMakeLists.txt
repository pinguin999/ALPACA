cmake_minimum_required(VERSION 3.12)

# Project settings
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ".")
set(PROJECT_UNIT_TESTS_NAME alpaca_unit_tests)

# Gather the header and source files
file(GLOB UNIT_TESTS_INC_FILES ${PROJECT_SOURCE_DIR}/test/*.hpp)
file(GLOB UNIT_TESTS_SRC_FILES ${PROJECT_SOURCE_DIR}/test/*.cpp)

# Include paths
set(UNIT_TESTS_INCLUDES ${PROJECT_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/src
    ./../src
    ./../subprojects/schnacker/src
    ../subprojects/jngl/include/public)

# Assign the include directories
include_directories(${UNIT_TESTS_INCLUDES})
include_directories(../subprojects/gifanimcplusplus)


#Remove games main.cpp
get_filename_component(full_path_test_cpp ${PROJECT_SOURCE_DIR}/src/main.cpp ABSOLUTE)
message("${full_path_test_cpp}")

list(REMOVE_ITEM SOURCES "${full_path_test_cpp}")

# Build unit tests
#add_executable(${PROJECT_UNIT_TESTS_NAME} ${UNIT_TESTS_SRC_FILES} $<TARGET_OBJECTS:demo_objects> )
add_executable(${PROJECT_UNIT_TESTS_NAME} ${UNIT_TESTS_SRC_FILES} ${SOURCES})

if (APPLE)
    find_library(CoreServices CoreServices)
    target_link_libraries(${PROJECT_UNIT_TESTS_NAME} PRIVATE jngl schnacker spine-cpp $<$<CONFIG:Debug>:${CoreServices}>)
else()
    target_link_libraries(${PROJECT_UNIT_TESTS_NAME} jngl schnacker spine-cpp)
endif()

enable_testing()
add_test(AlpacaTest ${PROJECT_UNIT_TESTS_NAME})
