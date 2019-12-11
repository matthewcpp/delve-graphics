project(delve-graphics)
cmake_minimum_required(VERSION 3.0)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(MACOS TRUE)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
message("${CMAKE_MODULE_PATH}")

if (DEFINED VULKAN_SDK)
	message("Using Vulcan SDK: ${VULKAN_SDK}")
else()
	message(FATAL_ERROR "You you must specify the VULKAN_SDK path")
endif()

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

find_package(Vulkan REQUIRED)

add_executable(vulkantest main.cpp)
set_target_properties(vulkantest PROPERTIES 
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON)

target_link_libraries(vulkantest Vulkan::Vulkan CONAN_PKG::glfw CONAN_PKG::glm)
