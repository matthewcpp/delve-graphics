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
find_package(nlohmann_json REQUIRED)

add_executable(vulkantest 
    src/main.cpp
    include/vkdev/assets.h src/assets.cpp
    include/vkdev/bounds.h
    include/vkdev/buffer.h src/buffer.cpp
    include/vkdev/commandpool.h src/commandpool.cpp
    include/vkdev/device.h src/device.cpp
    include/vkdev/descriptor.h src/descriptor.cpp
    include/vkdev/image.h src/image.cpp
    include/vkdev/instance.h src/instance.cpp
    include/vkdev/material.h
    include/vkdev/mesh.h src/mesh.cpp
    include/vkdev/pipeline.h src/pipeline.cpp
    include/vkdev/queue.h src/queue.cpp
    include/vkdev/rendercommand.h src/rendercommand.cpp
    include/vkdev/rendertarget.h src/rendertarget.cpp
    include/vkdev/shader.h src/shader.cpp
    include/vkdev/swapchain.h src/swapchain.cpp
    include/vkdev/texture.h src/texture.cpp
    include/vk_mem_alloc.h src/vk_mem_alloc.cpp
    include/vkdev/window.h src/window.cpp
)
set_target_properties(vulkantest PROPERTIES 
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)

target_link_libraries(vulkantest Vulkan::Vulkan glfw::glfw glm::glm stb::stb nlohmann_json::nlohmann_json)
target_include_directories(vulkantest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/compile_all_shaders.in ${CMAKE_CURRENT_BINARY_DIR}/compile_all_shaders.sh @ONLY)
