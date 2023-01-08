cmake_minimum_required(VERSION 3.12)

project(pac LANGUAGES C CXX)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported OUTPUT error)

set(CMAKE_CXX_STANDARD 17)
if(NOT MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color")
endif()


if( ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
	set(USE_FLAGS "-s USE_SDL=2 -s USE_FREETYPE=1 -s USE_VORBIS=1 -s USE_LIBPNG=1 -s DISABLE_EXCEPTION_CATCHING=0 ")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${USE_FLAGS}")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${USE_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${USE_FLAGS} -s ASSERTIONS=1 -s FULL_ES3=1 -s INITIAL_MEMORY=1024MB --preload-file data ") #-s ALLOW_MEMORY_GROWTH=1 --source-map-base https://pac4.gitlab.io/pac/ -g
	if (CMAKE_BUILD_TYPE MATCHES "Debug")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O0 -g --source-map-base http://[::]:8000/")  # https://developer.chrome.com/blog/wasm-debugging-2020/ and https://groups.google.com/g/emscripten-discuss/c/DEmpyGoq6kE --source-map-base https://pac4.gitlab.io/pac/
	endif()
	SET(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

add_subdirectory(subprojects/jngl)
add_subdirectory(subprojects/spine-runtimes/spine-c)
add_subdirectory(subprojects/schnacker/)

set(PAC_SANITIZE_ADDRESS_DEFAULT ON)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" OR ANDROID OR CMAKE_BUILD_TYPE MATCHES "Release")
	set(PAC_SANITIZE_ADDRESS_DEFAULT OFF)
endif()
option(PAC_SANITIZE_ADDRESS "Enable AddressSanitizer" ${PAC_SANITIZE_ADDRESS_DEFAULT})
if(PAC_SANITIZE_ADDRESS AND NOT EMSCRIPTEN)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
	set (CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
endif()

file(GLOB SOURCES CONFIGURE_DEPENDS
	src/*.cpp
	src/input/*.cpp
	src/dialog/*.cpp
)

if (NOT CMAKE_BUILD_TYPE MATCHES "Release" AND NOT ANDROID AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
	file(GLOB SOURCESGIF CONFIGURE_DEPENDS
		subprojects/gifanimcplusplus/*.cpp)
	list (APPEND SOURCES ${SOURCESGIF})
endif()

if(ANDROID)
	add_library(pac SHARED ${SOURCES})
	target_link_libraries(pac PRIVATE
		android
		native_app_glue
		EGL
		GLESv3
		OpenSLES
		log
		z
	)
else()
	if(WIN32)
		add_executable(pac ${SOURCES})
	else()
		add_executable(pac ${SOURCES})
	endif()
endif()

target_link_libraries(pac PRIVATE
	jngl spine-c schnacker
)

if(MSVC)
  add_custom_command(TARGET pac COMMAND ${CMAKE_COMMAND} -E copy
    ${PROJECT_SOURCE_DIR}/subprojects/jngl/lib/msvc/x86_64/epoxy-0.dll
    "$<TARGET_FILE_DIR:pac>")
endif()


# Add Tests
if (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" AND NOT IOS AND  NOT ANDROID AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
	add_subdirectory(test)
endif()

if(IOS)
	file(GLOB ios_data "data/*")
	list(APPEND ios_data ios/Storyboard.storyboard ios/logo_front_fullHD.png)
	set_source_files_properties(${ios_data} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
	target_sources(pac PRIVATE subprojects/jngl/src/ios/test/main.mm ios/Media.xcassets ${ios_data})
	set_source_files_properties(ios/Media.xcassets PROPERTIES
		MACOSX_PACKAGE_LOCATION Resources
	)
	# https://github.com/wrld3d/wrld-example-app/blob/72218e1783c90b7ae1470f41dca1c9eedd6e2678/ios/CMakeLists.txt
	set_target_properties(
		pac
		PROPERTIES
		MACOSX_BUNDLE YES
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/ios/Info.plist"
		XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.bixense.pac"
		XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
		XCODE_ATTRIBUTE_INSTALL_PATH "/Applications" # otherwise Xcode will create an empty Archive
		XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
		# XCODE_ATTRIBUTE_DEVELOPMENT_TEAM 4DMBNMNYSA
	)
	# set_target_properties(freetype PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/out/library)
	# set_target_properties(jngl PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/out/library)
	# set_target_properties(ogg PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/out/library)
	# set_target_properties(vorbis PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/out/library)
	# set_target_properties(vorbisfile PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/out/library)
	# set_target_properties(webpdecode PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/out/library)
	# set_target_properties(webpdspdecode PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/out/library)
	# set_target_properties(webputilsdecode PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/out/library)
endif()

file(GLOB GFX_FILES RELATIVE ${CMAKE_SOURCE_DIR}/data CONFIGURE_DEPENDS data/*.webp data/*.png
                                                                        data/*/*.webp)
file(GLOB SFX_FILES RELATIVE ${CMAKE_SOURCE_DIR}/data data/sfx/*.ogg)
# configure_file(src/engine/datafiles.hpp.in include/datafiles.hpp @ONLY)
target_include_directories(pac PRIVATE ${CMAKE_BINARY_DIR}/include
										subprojects/spine-runtimes/spine-c/spine-c/include
										subprojects/schnacker/src)
if (NOT CMAKE_BUILD_TYPE MATCHES "Release" AND NOT ANDROID AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
	target_include_directories(pac PRIVATE subprojects/gifanimcplusplus)
endif()

if(WIN32)
	target_compile_definitions(pac PRIVATE _USE_MATH_DEFINES)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	set_target_properties(pac PROPERTIES WIN32_EXECUTABLE 1)
endif()

if(ANDROID)
	target_include_directories(pac PRIVATE
		${ANDROID_NDK}/sources/android/native_app_glue)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	if(lto_supported)
		message(STATUS "IPO / LTO enabled")
		set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
		set_property(TARGET pac jngl spine-c schnacker PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
	else()
		message(STATUS "IPO / LTO not supported: <${error}>")
	endif()
endif()

if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
	target_compile_options(pac PUBLIC "/ZI")
	target_link_options(pac PUBLIC "/INCREMENTAL")
	set(lto_supported FALSE)
endif()

if(MSVC)
	target_compile_options(pac PRIVATE /W4)
	# TODO add /WX
else()
	if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
		target_compile_options(pac PRIVATE -std=c++20 -Wall -Wextra -pedantic -Werror -Wno-long-long -Wunused -Wnon-virtual-dtor -Wno-align-mismatch )
	#    TODO Add -Wshadow -Wconversion
	# -std=c++20 -O3 -fsanitize=fuzzer,address,undefined -fPIE
	endif()
endif()
