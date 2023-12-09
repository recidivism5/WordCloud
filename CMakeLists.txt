cmake_minimum_required(VERSION 3.5)
project (Car C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinydir)

add_subdirectory(third_party/Boxer)

SET(PNG_LINK_ZLIB_STATIC ON CACHE BOOL "idk what the fuck this string is for")
add_subdirectory(third_party/deps-zlib-libpng)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/third_party/deps-zlib-libpng/zlib)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/deps-zlib-libpng/zlib)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/third_party/deps-zlib-libpng/libpng)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/deps-zlib-libpng/libpng)

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})

set(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW lib only")
set(GLFW_INSTALL OFF CACHE BOOL "GLFW lib only")
add_subdirectory(third_party/glfw)

add_subdirectory(third_party/glad)

add_subdirectory(third_party/openal-soft)

add_subdirectory(third_party/cglm)

include_directories(third_party/fast_obj)
add_subdirectory(third_party/fast_obj)

include_directories(include)
file(GLOB_RECURSE CAR-SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
	 
add_executable( Car ${CAR-SRC} )
if(MSVC)
	set_target_properties(
		Car
		PROPERTIES
			LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE"
			LINK_FLAGS_RELEASE "/SUBSYSTEM:windows /ENTRY:mainCRTStartup"
			LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:windows /ENTRY:mainCRTStartup"
			LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:windows /ENTRY:mainCRTStartup"
	)
endif()

target_link_libraries( Car Boxer png16_static ${OPENGL_LIBRARIES} glfw glad OpenAL::OpenAL cglm fast_obj_lib)
if( MSVC )
	if(${CMAKE_VERSION} VERSION_LESS "3.6.0") 
		message( "\n\t[ WARNING ]\n\n\tCMake version lower than 3.6.\n\n\t - Please update CMake and rerun; OR\n\t - Manually set 'Car' as StartUp Project in Visual Studio.\n" )
	else()
		set_property( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Car )
	endif()
endif()
