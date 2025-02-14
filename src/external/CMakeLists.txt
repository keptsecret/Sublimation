cmake_minimum_required(VERSION 3.25)

###########################################################################
# assimp

set (ASSIMP_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/assimp/include ${CMAKE_CURRENT_BINARY_DIR}/assimp/include PARENT_SCOPE)

set (BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set (ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set (ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set (ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set (ASSIMP_BUILD_ASSIMP_VIEW OFF CACHE BOOL "" FORCE)

add_subdirectory (assimp)

set_property (TARGET assimp PROPERTY FOLDER "external")

###########################################################################
# glfw

set (GLFW_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include PARENT_SCOPE)

set (GLFW_LIBRARY_TYPE STATIC CACHE STRING "" FORCE)
set (GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set (GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set (GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# in the case of hybrid graphics on Windows
if (USE_HYBRID_GRAPHICS)
    message (STATUS "Setting GLFW to compile for hybrid graphics")
    set (GLFW_USE_HYBRID_HPG ON CACHE BOOL "" FORCE)
endif ()

add_subdirectory (glfw)
set_property (TARGET glfw PROPERTY FOLDER "external")

###########################################################################
# spirv-cross

set (SPIRV_CROSS_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/SPIRV-Cross/include PARENT_SCOPE)

set (SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set (SPIRV_CROSS_SKIP_INSTALL OFF CACHE BOOL "" FORCE)
set (SPIRV_CROSS_STATIC ON CACHE BOOL "" FORCE)
set (SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)

add_subdirectory (spirv-cross)

set_property (TARGET spirv-cross-core PROPERTY FOLDER "external")

###########################################################################
# volk

set (VOLK_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/volk PARENT_SCOPE)

if (WIN32)
   set (VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
endif()
add_subdirectory (volk)

set_property (TARGET volk PROPERTY FOLDER "external")

###########################################################################
# vma

set (VMA_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/vma/include PARENT_SCOPE)

###########################################################################
# glm

set (GLM_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/glm PARENT_SCOPE)

###########################################################################
# stb

set (STB_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/stb PARENT_SCOPE)
