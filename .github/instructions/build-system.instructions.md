---
applyTo: "CMakeLists.txt,src/CMakeLists.txt"
---

# Build System Instructions

> Context-aware instructions for modifying the CMake build configuration.

## Before You Touch Build Files

1. Read `CMakeLists.txt` (root) and `src/CMakeLists.txt` fully.
2. Understand the dual-target setup: root creates `juceDjApp`, `src/CMakeLists.txt` conditionally adds sources to it or creates a standalone `JUCELoopStation`.

## Build Architecture

```
CMakeLists.txt (root)
  ├─ FetchContent: JUCE, TagLib, xwax
  ├─ juce_add_gui_app(juceDjApp)
  ├─ Binary data (LoopStationBinaryData)
  ├─ GLOB_RECURSE for src/**
  └─ target_link_libraries (JUCE modules + TagLib)

src/CMakeLists.txt
  ├─ SOURCE_FILES list (explicit, not globbed)
  ├─ If juceDjApp target exists → adds to it
  └─ Else → creates JUCELoopStation standalone target
```

## Adding New Source Files

Add both the `.h` and `.cpp` to the `SOURCE_FILES` list in `src/CMakeLists.txt`:

```cmake
set(SOURCE_FILES
    Main.cpp
    MainComponent.cpp
    # ... existing files ...
    YourNewFile.cpp
)
```

The root `CMakeLists.txt` also has a `GLOB_RECURSE` that picks up `src/**`, but the explicit list in `src/CMakeLists.txt` takes precedence for the `target_sources` call.

## Adding New Binary Assets

1. Place the asset file in `assets/`
2. Add conditional registration in **both** CMake files:

```cmake
# In BOTH CMakeLists.txt and src/CMakeLists.txt:
if(EXISTS "${CMAKE_SOURCE_DIR}/assets/yourAsset.svg")
    list(APPEND LoopStationBinaryData_SOURCES "${CMAKE_SOURCE_DIR}/assets/yourAsset.svg")
endif()
```

3. Access in code via `BinaryData::yourAsset_svg` (JUCE generates the symbol name from the filename).

## Adding New Dependencies

Use `FetchContent` in the root `CMakeLists.txt`:

```cmake
FetchContent_Declare(
    newlib
    GIT_REPOSITORY https://github.com/...
    GIT_TAG origin/main
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(newlib)
```

Then link: `target_link_libraries(juceDjApp PRIVATE newlib)`.

## Linked JUCE Modules

Currently linked: `juce_analytics`, `juce_audio_basics`, `juce_audio_devices`, `juce_core`, `juce_data_structures`, `juce_graphics`, `juce_gui_basics`, `juce_audio_utils`, `juce_dsp`.

Note: `juce_gui_extra` is disabled to avoid WebKit dependency on Linux. If you need it, ensure WebKit2 dev packages are installed.

## Build Commands

```bash
cmake -B build -G Ninja                    # Configure
cmake --build build                        # Build
cmake --build build --target clean         # Clean
cmake -B build -DCMAKE_BUILD_TYPE=Release  # Release build
```
