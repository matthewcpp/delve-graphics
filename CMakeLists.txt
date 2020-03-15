project(vkdev)
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
find_package(stb REQUIRED)

add_executable(vulkantest 
    src/main.cpp
    include/vkdev/bounds.h
    include/vkdev/buffer.h src/buffer.cpp
    include/vkdev/command.h src/command.cpp
    include/vkdev/device.h src/device.cpp
    include/vkdev/image.h src/image.cpp
    include/vkdev/instance.h src/instance.cpp
    include/vkdev/mesh.h src/mesh.cpp
    include/vkdev/queue.h src/queue.cpp
    include/vkdev/shader.h src/shader.cpp
    include/vkdev/swapchain.h src/swapchain.cpp
    include/vkdev/texture.h src/texture.cpp
    include/vkdev/window.h src/window.cpp
)
set_target_properties(vulkantest PROPERTIES 
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)

target_link_libraries(vulkantest Vulkan::Vulkan glfw::glfw glm::glm stb::stb)
target_include_directories(vulkantest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/compile_all_shaders.in ${CMAKE_CURRENT_BINARY_DIR}/compile_all_shaders.sh @ONLY)
