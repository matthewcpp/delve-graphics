if (WIN32)
    set(vk_include_path Include)
    set(vk_library_name vulkan-1.lib)
    set(vk_library_path Lib)

    if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
        set(vk_library_path ${vk_library_path}32)
    endif()
    
elseif(MACOS)
    set(vk_include_path MoltenVK/include)
    set(vk_library_name MoltenVK)
    set(vk_library_path MoltenVK/macOS/dynamic)
elseif (UNIX)
    set(vk_include_path x86_64/include)
    set(vk_library_name vulkan)
    set(vk_library_path x86_64/lib)
endif()

find_path(Vulkan_INCLUDE_DIR
        NAMES vulkan/vulkan.h
        HINTS ${VULKAN_SDK}
        PATH_SUFFIXES ${vk_include_path})

find_library(Vulkan_LIBRARY
        NAMES ${vk_library_name}
        HINTS ${VULKAN_SDK}
        PATH_SUFFIXES ${vk_library_path})

set(Vulkan_LIBRARIES ${Vulkan_LIBRARY})
set(Vulkan_INCLUDE_DIRS ${Vulkan_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Vulkan
    DEFAULT_MSG
    Vulkan_LIBRARY Vulkan_INCLUDE_DIR)

mark_as_advanced(Vulkan_INCLUDE_DIR Vulkan_LIBRARY)

if(Vulkan_FOUND AND NOT TARGET Vulkan::Vulkan)
    add_library(Vulkan::Vulkan UNKNOWN IMPORTED)
    set_target_properties(Vulkan::Vulkan PROPERTIES
        IMPORTED_LOCATION "${Vulkan_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIRS}")
endif()