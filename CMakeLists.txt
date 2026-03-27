cmake_minimum_required(VERSION 3.16)
project(opencluster LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ── LVGL configuration ──────────────────────────────────────────────────
# Point LVGL at our desktop lv_conf.h (LV_CONF_INCLUDE_SIMPLE mode).
set(LV_BUILD_CONF_DIR "${CMAKE_CURRENT_SOURCE_DIR}/config/desktop" CACHE PATH "" FORCE)

# Disable demos/examples we don't need.
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Tell LVGL's dependency resolver that SDL is required.
set(CONFIG_LV_USE_SDL ON CACHE BOOL "" FORCE)

# ── Find dependencies BEFORE adding LVGL ────────────────────────────────
# SDL2 must be found first so its include paths are available during LVGL
# compilation (os_desktop.cmake does NOT resolve dependencies automatically).
find_package(SDL2 REQUIRED)
find_package(Threads REQUIRED)

# ── Add LVGL as a subdirectory ──────────────────────────────────────────
add_subdirectory(components/lvgl)

# Link SDL2 and pthreads into LVGL.
if(TARGET SDL2::SDL2)
    target_link_libraries(lvgl PUBLIC SDL2::SDL2)
else()
    target_include_directories(lvgl PUBLIC ${SDL2_INCLUDE_DIRS})
    target_link_libraries(lvgl PUBLIC ${SDL2_LIBRARIES})
endif()
target_link_libraries(lvgl PUBLIC Threads::Threads)

# ── Include paths shared by all project targets ─────────────────────────
set(PROJECT_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/hal/include
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/skins/e46
)

# ── Shared source files (core + HAL desktop + skins) ────────────────────
set(CORE_SOURCES
    core/vehicle_data.c
    core/skin_registry.c
    core/layout.c
)

# Sources used by can_simulator (no LVGL dependency)
set(CORE_SOURCES_HEADLESS
    core/vehicle_data.c
)

set(HAL_DESKTOP_SOURCES
    hal/desktop/hal_can_udp.c
    hal/desktop/hal_platform_posix.c
)

set(SKIN_SOURCES
    skins/tachometer/skin_tachometer.c
    skins/speedometer/skin_speedometer.c
    skins/fuel_gauge/skin_fuel_gauge.c
    skins/coolant_temp/skin_coolant_temp.c
    skins/e46/e46_fuel.c
    skins/e46/e46_speedometer.c
    skins/e46/e46_tachometer.c
    skins/e46/e46_coolant.c
)

# Sources that require LVGL (display, input, skins)
set(HAL_DESKTOP_LVGL_SOURCES
    hal/desktop/hal_display_sdl.c
    hal/desktop/hal_input_sdl.c
)

# ── Compiler warnings ──────────────────────────────────────────────────
# Strict warnings for our code, but not for LVGL.
set(PROJECT_WARNING_FLAGS -Wall -Wextra -Werror)

# ── display_node executable ─────────────────────────────────────────────
add_executable(display_node
    apps/display_node/main.c
    ${CORE_SOURCES}
    ${HAL_DESKTOP_SOURCES}
    ${HAL_DESKTOP_LVGL_SOURCES}
    ${SKIN_SOURCES}
)
target_include_directories(display_node PRIVATE ${PROJECT_INCLUDE_DIRS})
target_link_libraries(display_node PRIVATE lvgl m)
target_compile_options(display_node PRIVATE ${PROJECT_WARNING_FLAGS})

# ── can_simulator executable (headless, no LVGL) ────────────────────────
add_executable(can_simulator
    apps/can_simulator/main.c
    ${CORE_SOURCES_HEADLESS}
    ${HAL_DESKTOP_SOURCES}
)
target_include_directories(can_simulator PRIVATE ${PROJECT_INCLUDE_DIRS})
target_link_libraries(can_simulator PRIVATE m Threads::Threads)
target_compile_options(can_simulator PRIVATE ${PROJECT_WARNING_FLAGS})
