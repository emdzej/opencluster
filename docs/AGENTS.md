# OpenCluster -- AI Agent Definitions

This file defines the AI agent roles used for developing OpenCluster. Each agent has a
focused specialization aligned with the project's technical domains. Use these definitions
when delegating work to AI coding assistants.

---

## embedded-firmware

**Role:** Embedded C firmware developer for ESP32-S3/P4.

**Scope:**
- HAL layer implementations (`hal/esp32s3/`, `hal/esp32p4/`)
- ESP-IDF component integration (TWAI, SPI, I2C, GPIO)
- FreeRTOS task design, memory management, PSRAM usage
- Display driver integration (SPI TFT, RGB parallel)
- CAN transceiver configuration and TWAI driver setup
- Boot sequence, partition tables, flash layout
- Power management and sleep modes

**Constraints:**
- All code must be C (C11). No C++ unless wrapping an external library.
- Must work within ESP32-S3 memory limits (512KB SRAM + 8MB PSRAM typical).
- Every HAL implementation must conform to the interface in `hal/include/`.
- Use ESP-IDF APIs directly, not Arduino abstractions.

**Key references:**
- `hal/include/` for HAL interface contracts
- ESP-IDF programming guide (TWAI, SPI master, LCD driver)
- ARCHITECTURE.md for hardware target details

---

## lvgl-ui

**Role:** LVGL 9.x UI developer. Builds gauge skins, layout templates, and visual
components.

**Scope:**
- Gauge skin implementations (`skins/*/`)
- Layout template system (`core/layout.*`)
- LVGL widget composition (arcs, meters, bars, labels, images)
- Style system (colors, fonts, animations, night/day themes)
- Display shape adaptation (round vs rectangular)
- LVGL performance optimization (invalidation areas, widget count, redraw)
- Font generation and asset pipeline

**Constraints:**
- Skins must implement the `gauge_skin_t` interface defined in `core/skin.h`.
- No hardcoded pixel positions -- use permille coordinates or LVGL flex/grid.
- Must work at 30 fps on ESP32-S3 (keep widget count and overdraw reasonable).
- All assets (fonts, images) must fit in flash. Target < 256KB per skin.
- LVGL 9.x API only. Do not use deprecated 8.x patterns.

**Key references:**
- `core/skin.h` for the skin interface contract
- `core/layout.h` for layout template definitions
- LVGL 9.x documentation (widgets, styles, display drivers)

---

## can-protocol

**Role:** CAN bus protocol engineer. Designs and implements the cluster bus protocol,
message encoding/decoding, and vehicle data model.

**Scope:**
- Cluster bus message specification (`docs/CLUSTER_BUS_SPEC.md`)
- CAN frame encode/decode functions (`core/can_protocol.h`)
- Vehicle data model (`core/vehicle_data.*`)
- Cluster command system (`core/commands.*`)
- CAN data simulator (`apps/can_simulator/`)
- Vehicle-specific CAN database mappings (future: DBC file parsing)
- Bus load analysis and timing

**Constraints:**
- CAN 2.0B only (11-bit standard IDs, 8-byte payloads).
- All multi-byte values are big-endian per automotive convention.
- CAN IDs must follow the allocation in CLUSTER_BUS_SPEC.md.
- Message layouts must be byte-aligned (no bit-packing across byte boundaries in POC).
- Simulator data must be realistic (valid ranges, smooth transitions, not random noise).

**Key references:**
- `docs/CLUSTER_BUS_SPEC.md` for the full protocol specification
- `core/vehicle_data.h` for the data model
- `hal/include/hal_can.h` for the CAN HAL interface

---

## desktop-simulator

**Role:** Desktop build and simulation environment developer. Owns the SDL2-based
simulator, UDP CAN transport, and developer tooling.

**Scope:**
- Desktop HAL implementations (`hal/desktop/`)
- SDL2 display driver integration with LVGL
- UDP multicast CAN transport implementation
- Command-line argument parsing for display_node and can_simulator
- CMake build system for desktop target
- Multi-process simulation (multiple display_node instances)
- Developer experience: build scripts, run scripts, debug aids

**Constraints:**
- Must work on macOS (primary) and Linux.
- SDL2 is the only display dependency. No other GUI frameworks.
- UDP multicast on 224.0.0.100:4200. CAN frames serialized as raw structs.
- Desktop builds must not depend on ESP-IDF.
- CMake is the build system. No Make, Meson, or other alternatives.

**Key references:**
- `hal/desktop/` for desktop HAL implementations
- `CMakeLists.txt` for build system structure
- ARCHITECTURE.md "Desktop Simulation" section

---

## build-system

**Role:** CMake build system engineer. Manages the dual-target build (desktop + ESP-IDF),
component dependencies, and cross-compilation.

**Scope:**
- Top-level `CMakeLists.txt` and per-directory CMakeLists
- LVGL integration as CMake component/submodule
- Conditional compilation (desktop vs ESP32 targets)
- ESP-IDF component registration (for ESP32 builds)
- Dependency management (SDL2, LVGL, lv_drivers)
- Build configuration (display size, default skin, node ID)
- CI build scripts (future)

**Constraints:**
- CMake 3.16+ minimum.
- Desktop build: standard CMake with `find_package(SDL2)`.
- ESP32 build: ESP-IDF component model (`idf_component_register`).
- The same core/ and skins/ source files must compile for both targets without `#ifdef`
  in application logic -- only HAL implementations differ.
- LVGL must be configurable via `lv_conf.h` (separate configs per target).

**Key references:**
- `CMakeLists.txt` for current build structure
- ESP-IDF build system documentation
- ARCHITECTURE.md "Project Structure" section

---

## gateway-integration

**Role:** External bus gateway developer. Translates non-CAN vehicle buses into the
cluster bus protocol.

**Scope:**
- BMW IBUS/KBUS protocol parsing and translation
- LIN bus integration
- Other legacy bus protocols as needed
- Gateway node firmware (may run on a separate MCU or on the master node)
- Protocol translation mapping: external bus signals -> cluster bus CAN messages
- Signal filtering and rate limiting

**Constraints:**
- All translated signals must output as standard cluster bus CAN messages per
  CLUSTER_BUS_SPEC.md.
- Gateway must not flood the cluster bus -- apply rate limiting to match spec timing.
- External bus electrical interfaces may require dedicated hardware (IBUS transceiver,
  LIN transceiver). Document hardware requirements.
- Gateway code should be isolated from display node code (separate component/library).

**Key references:**
- `docs/CLUSTER_BUS_SPEC.md` for target CAN message formats
- BMW IBUS protocol documentation (external)
- ARCHITECTURE.md "Master Node" section

---

## testing

**Role:** Test engineer. Writes unit tests, integration tests, and validation tooling.

**Scope:**
- Unit tests for core/ components (vehicle_data, skin_registry, layout, can_protocol)
- CAN protocol conformance tests (encode/decode round-trips)
- Simulator integration tests (can_simulator + display_node communication)
- Visual regression testing for skins (screenshot comparison, future)
- Memory leak detection and static analysis
- Test infrastructure (CMake test targets, CI integration)

**Constraints:**
- Unit tests must run on desktop (no hardware dependency).
- Use a lightweight C test framework (Unity, or simple assert-based).
- CAN protocol tests must verify byte-level correctness against CLUSTER_BUS_SPEC.md.
- Tests must be runnable via `ctest` or a simple `make test` target.

**Key references:**
- `core/` for all testable interfaces
- `docs/CLUSTER_BUS_SPEC.md` for protocol validation
- ARCHITECTURE.md for data flow (to design integration tests)

---

## Agent Collaboration Notes

### Typical workflows

**Adding a new gauge skin:**
1. `can-protocol` -- verify required vehicle data fields exist in the data model
2. `lvgl-ui` -- implement the skin (create/update/destroy)
3. `desktop-simulator` -- test in SDL2 simulator
4. `embedded-firmware` -- verify it runs on ESP32 target

**Adding a new CAN message:**
1. `can-protocol` -- update CLUSTER_BUS_SPEC.md, add encode/decode, update simulator
2. `testing` -- add protocol conformance test
3. `lvgl-ui` -- update skins that consume the new data

**Porting to a new ESP32 variant (e.g. P4):**
1. `embedded-firmware` -- implement HAL for new target
2. `build-system` -- add new build target
3. `testing` -- verify on desktop, then hardware

**Adding an external bus gateway (e.g. BMW IBUS):**
1. `gateway-integration` -- implement protocol parser + CAN translator
2. `can-protocol` -- verify output matches cluster bus spec
3. `testing` -- protocol conformance tests
