cmake_minimum_required (VERSION 3.25)
project (sublimation)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include(CheckCXXSourceCompiles)
include(CheckCXXSourceRuns)
include(CMakeDependentOption)

########################################
# Vulkan

if (NOT CMAKE_VERSION VERSION_LESS 3.7.0)
    message(STATUS "Using module to find Vulkan")
    find_package(Vulkan)
endif ()

if (WIN32)
    if (NOT Vulkan_FOUND)
        find_library(Vulkan_LIBRARY NAMES vulkan-1 vulkan PATHS ${CMAKE_SOURCE_DIR}/libs/vulkan)
        if (Vulkan_LIBRARY)
            set(Vulkan_FOUND ON)
            MESSAGE("Using bundled Vulkan library version")
        endif ()
    endif ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
elseif (LINUX)
    if (NOT Vulkan_FOUND)
        find_library(Vulkan_LIBRARY NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_SOURCE_DIR}/libs/vulkan" REQUIRED)
        if (Vulkan_LIBRARY)
            set(Vulkan_FOUND ON)
            MESSAGE("Using bundled Vulkan library version")
        endif ()
    endif ()
    find_package(Threads REQUIRED)
    if (USE_D2D_WSI)
        MESSAGE("Using direct to display extension...")
        add_definitions(-D_DIRECT2DISPLAY)
    elseif (USE_DIRECTFB_WSI)
        find_package(DirectFB REQUIRED)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_DIRECTFB_EXT")
        include_directories(${DIRECTFB_INCLUDE_DIR})
    elseif (USE_WAYLAND_WSI)
        find_program(PKG_CONFIG pkg-config)
        if (NOT PKG_CONFIG)
            message(FATAL_ERROR "pkg-config binary not found")
        endif ()
        find_package(Wayland REQUIRED)
        if (NOT WAYLAND_FOUND)
            message(FATAL_ERROR "Wayland development package not found")
        endif ()
        pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
        if (NOT WAYLAND_PROTOCOLS_FOUND)
            message(FATAL_ERROR "Wayland protocols package not found")
        endif ()
        find_program(WAYLAND_SCANNER wayland-scanner)
        if (NOT WAYLAND_SCANNER)
            message(FATAL_ERROR "wayland-scanner binary not found")
        endif ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WAYLAND_KHR")
        include_directories(${WAYLAND_INCLUDE_DIR})
        pkg_get_variable(protocol_dir wayland-protocols pkgdatadir)
        execute_process(COMMAND ${WAYLAND_SCANNER} client-header ${protocol_dir}/stable/xdg-shell/xdg-shell.xml ${CMAKE_BINARY_DIR}/xdg-shell-client-protocol.h
                COMMAND ${WAYLAND_SCANNER} private-code ${protocol_dir}/stable/xdg-shell/xdg-shell.xml ${CMAKE_BINARY_DIR}/xdg-shell-protocol.c)
        include_directories(${CMAKE_BINARY_DIR})
    elseif (USE_HEADLESS)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_HEADLESS_EXT")
    else (USE_D2D_WSI)
        find_package(XCB REQUIRED)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_XCB_KHR")
    endif (USE_D2D_WSI)
elseif (APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_MACOS_MVK -DVK_EXAMPLE_XCODE_GENERATED")
endif ()

if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Could not find Vulkan library!")
else ()
    message(STATUS ${Vulkan_LIBRARY})
endif ()

########################################

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX")

########################################
# thirdparty libraries

set(BUILD_SHARED_LIBS OFF)
cmake_dependent_option(USE_HYBRID_GRAPHICS "Compile GLFW for systems with hybrid graphics" OFF
        "WIN32" OFF)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/external)

########################################

set(SUBLIMATION_CORE_HEADERS
        src/core/engine.h
        )

set(SUBLIMATION_CORE_SOURCE
        src/core/engine.cpp
        )

set(SUBLIMATION_GRAPHICS_HEADERS
        src/graphics/vulkan/rendering_device.h
        src/graphics/vulkan/vulkan_context.h
        src/graphics/vulkan/swapchain.h
        src/graphics/vulkan/pipeline.h
        src/graphics/vulkan/render_target.h
        src/graphics/vulkan/descriptor.h
        src/graphics/vulkan/command_buffer.h
        src/graphics/vulkan/buffer.h
        src/graphics/vulkan/texture.h
        src/graphics/vulkan/utils.h)

set(SUBLIMATION_GRAPHICS_SOURCE
        src/graphics/vulkan/rendering_device.cpp
        src/graphics/vulkan/vulkan_context.cpp
        src/graphics/vulkan/swapchain.cpp
        src/graphics/vulkan/render_target.cpp
        src/graphics/vulkan/descriptor.cpp
        src/graphics/vulkan/command_buffer.cpp
        src/graphics/vulkan/buffer.cpp
        src/graphics/vulkan/texture.cpp
        src/graphics/vulkan/utils.cpp)

set(SUBLIMATION_SCENE_HEADERS
        )

set(SUBLIMATION_SCENE_SOURCE
        )

add_library(sublimation_lib STATIC
        ${SUBLIMATION_CORE_HEADERS}
        ${SUBLIMATION_CORE_SOURCE}
        ${SUBLIMATION_GRAPHICS_HEADERS}
        ${SUBLIMATION_GRAPHICS_SOURCE}
        ${SUBLIMATION_SCENE_HEADERS}
        ${SUBLIMATION_SCENE_SOURCE}
        )

target_include_directories(sublimation_lib PUBLIC
        src
        src/external
        ${ASSIMP_INCLUDE}
        ${Vulkan_INCLUDE_DIRS}
        ${VOLK_INCLUDE}
        ${VMA_INCLUDE}
        ${SPIRV_CROSS_INCLUDE}
        ${GLFW_INCLUDE}
        ${GLM_INCLUDE}
        ${STB_INCLUDE}
        )

target_compile_options(sublimation_lib PUBLIC ${SUBLIMATION_CXX_FLAGS})

if (WIN32)
    # Avoid a name clash when building on Visual Studio
    set_target_properties(sublimation_lib PROPERTIES OUTPUT_NAME libsublimation)
endif ()

set(SUBLIMATION_LIBS
        sublimation_lib
        assimp
        glfw
        spirv-cross-core
        volk
        Vulkan::Vulkan
        )

add_executable(sublimation_exe src/main.cpp)
add_executable(sublimation::sublimation_exe ALIAS sublimation_exe)

target_compile_options(sublimation_exe PRIVATE ${SUBLIMATION_CXX_FLAGS})
target_include_directories(sublimation_exe PRIVATE src src/external)
target_link_libraries(sublimation_exe ${SUBLIMATION_LIBS})
set_target_properties(sublimation_exe PROPERTIES OUTPUT_NAME sublimation)