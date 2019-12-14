project(delve-graphics)
cmake_minimum_required(VERSION 3.0)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(MACOS TRUE)
endif()

include(${CMAKE_BINARY_DIR}/conan_paths.cmake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

if (DEFINED VULKAN_SDK)
	message("Using Vulkan SDK: ${VULKAN_SDK}")
else()
	message(FATAL_ERROR "You you must specify the VULKAN_SDK path")
endif()

find_package(Vulkan REQUIRED)
find_package(glfw REQUIRED)
find_package(glm REQUIRED)

add_executable(vulkantest main.cpp)
set_target_properties(vulkantest PROPERTIES 
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON)

target_link_libraries(vulkantest Vulkan::Vulkan glfw::glfw glm::glm)
