cmake_minimum_required(VERSION 3.22.1)
project(neon_district LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(game-activity REQUIRED CONFIG)

set(GLSLC "${ANDROID_NDK}/shader-tools/${ANDROID_HOST_TAG}/glslc")
set(SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../shaders")
set(SHADER_OUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../assets/shaders")
file(MAKE_DIRECTORY "${SHADER_OUT_DIR}")

set(SHADERS text.vert text.frag)
set(SPIRV_FILES "")
foreach(S ${SHADERS})
    set(IN  "${SHADER_SRC_DIR}/${S}")
    set(OUT "${SHADER_OUT_DIR}/${S}.spv")
    add_custom_command(
        OUTPUT  "${OUT}"
        COMMAND "${GLSLC}" "${IN}" -o "${OUT}"
        DEPENDS "${IN}"
        COMMENT "Compiling ${S}"
    )
    list(APPEND SPIRV_FILES "${OUT}")
endforeach()
add_custom_target(compile_shaders ALL DEPENDS ${SPIRV_FILES})

add_library(neon SHARED
    platform/AndroidGlue.c
    platform/AndroidMain.cpp
    gfx/VulkanContext.cpp
    gfx/Swapchain.cpp
    gfx/TextRenderer.cpp
    gfx/Renderer.cpp
)

add_dependencies(neon compile_shaders)

target_include_directories(neon PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(neon PRIVATE
    game-activity::game-activity_static
    android
    log
    vulkan
)

target_link_options(neon PRIVATE
    "-u" "Java_com_google_androidgamesdk_GameActivity_initializeNativeCode"
)

target_compile_options(neon PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Werror>
)
