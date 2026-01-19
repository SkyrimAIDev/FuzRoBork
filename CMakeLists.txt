cmake_minimum_required(VERSION 3.21)

project(
  FuzRoBork
  VERSION 3.2.5
  DESCRIPTION "Text-to-Speech mod for Skyrim SE"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Deployment paths (optional environment variables)
if(DEFINED ENV{SKYRIM_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_FOLDER}/Data")
  set(OUTPUT_FOLDER "$ENV{SKYRIM_FOLDER}/Data")
endif()

if(DEFINED ENV{SKYRIM_MODS_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_MODS_FOLDER}")
  set(OUTPUT_FOLDER "$ENV{SKYRIM_MODS_FOLDER}/FuzRoBork")
endif()

# Find dependencies
find_package(CommonLibSSE CONFIG REQUIRED)
find_package(tinyxml2 CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_package(simpleini CONFIG REQUIRED)

# Source files
set(SOURCES
  src/Main.cpp
  src/FuzRoBorkInternals.cpp
  src/Hooks.cpp
)

set(HEADERS
  src/FuzRoBorkInternals.h
  src/Hooks.h
  src/VersionInfo.h
  src/BuildInfo.h
  src/PCH.h
)

# SME library includes (custom headers)
set(SME_HEADERS
  include/SME_Prefix.h
  include/INIManager.h
  include/StringHelpers.h
  include/MiscGunk.h
  include/MemoryHandler.h
  include/Functors.h
)

# Create plugin
add_commonlibsse_plugin(${PROJECT_NAME} SOURCES ${SOURCES} ${HEADERS} ${SME_HEADERS})

# Additional include directories
target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE
  tinyxml2::tinyxml2
  rapidjson
  simpleini::simpleini
  shlwapi.lib  # Windows Shell API
)

# Precompiled header
target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)

# Compiler options
target_compile_options(${PROJECT_NAME} PRIVATE
  /W4           # Warning level 4
  /WX-          # Warnings not as errors (gradual cleanup)
  /permissive-  # Strict conformance
  /Zc:preprocessor
  /EHsc
  /MP
)

# Preprocessor definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
  WIN32_LEAN_AND_MEAN
  NOMINMAX
  _UNICODE
  UNICODE
)

# Auto-deployment (if OUTPUT_FOLDER set)
if(DEFINED OUTPUT_FOLDER)
  set(DLL_FOLDER "${OUTPUT_FOLDER}/SKSE/Plugins")

  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DLL_FOLDER}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
    VERBATIM
  )

  # Copy PDB for debug builds
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(
      TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "$<TARGET_PDB_FILE:${PROJECT_NAME}>"
              "${DLL_FOLDER}/$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>"
      VERBATIM
    )
  endif()

  message(STATUS "Auto-deployment enabled to: ${DLL_FOLDER}")
endif()
