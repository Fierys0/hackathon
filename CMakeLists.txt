cmake_minimum_required(VERSION 3.16)

project(hackathon VERSION 0.1 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(default_build_type "Release")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING
      "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Enable Position Independent Code for all targets (required for shared libraries)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build Raylib as a static library" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
if(LINUX OR WIN32)
  add_subdirectory(engine/lib/raylib)
endif()
add_subdirectory(engine)

set_target_properties(fumbo_engine PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/hackathon
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/hackathon
)

add_executable(asset_pack EXCLUDE_FROM_ALL engine/tools/pack_assets.cpp)
target_include_directories(asset_pack PRIVATE
    engine
    engine/tools
    engine/lib/raylib/src
    src
)
if(WIN32)
  target_link_options(asset_pack PRIVATE -static)
endif()
set_target_properties(asset_pack PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tools
)

set(ASSET_PACK_OUTPUT ${CMAKE_BINARY_DIR}/hackathon/data.fpk)
set(ASSET_SRC_DIR ${CMAKE_SOURCE_DIR}/assets)

file(GLOB_RECURSE ASSET_SOURCES "${ASSET_SRC_DIR}/*")

add_custom_command(
    OUTPUT ${ASSET_PACK_OUTPUT}
    COMMAND $<TARGET_FILE:asset_pack> ${ASSET_SRC_DIR} ${ASSET_PACK_OUTPUT}
    DEPENDS asset_pack ${ASSET_SOURCES}
    COMMENT "Packing assets into data.fpk..."
    VERBATIM
)

add_custom_target(pack_assets DEPENDS ${ASSET_PACK_OUTPUT})

# Daftar semua file sumber game.
file(GLOB_RECURSE GAME_SOURCES
    src/core/*.cpp
    src/game/*.cpp
    src/states/*.cpp
    src/main.cpp
)

add_executable(${PROJECT_NAME} WIN32 ${GAME_SOURCES})

add_dependencies(${PROJECT_NAME} pack_assets)

set_target_properties(${PROJECT_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/hackathon
)

if(UNIX AND NOT APPLE)
  set_target_properties(${PROJECT_NAME} PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
    )
endif()

target_include_directories(${PROJECT_NAME}
    PRIVATE
        src
        src/lib
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        fumbo_engine
)

find_package(CURL)

# ─── CURL & HAS_CURL ─────────────────────────────────────────────────────────
if(WIN32 OR MINGW)
  # When building for Windows, use the bundled static curl from lib/windows/curl
  set(WINDOWS_CURL_DIR "${CMAKE_SOURCE_DIR}/lib/windows/curl")
  if(EXISTS "${WINDOWS_CURL_DIR}/include/curl/curl.h")
    message(STATUS "Using bundled Windows curl from ${WINDOWS_CURL_DIR}")
    add_compile_definitions(HAS_CURL=1)
    target_include_directories(${PROJECT_NAME} PRIVATE "${WINDOWS_CURL_DIR}/include")
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "${WINDOWS_CURL_DIR}/lib/libcurl.a"
        "${WINDOWS_CURL_DIR}/lib/libssl.a"
        "${WINDOWS_CURL_DIR}/lib/libcrypto.a"
        "${WINDOWS_CURL_DIR}/lib/libz.a"
        "${WINDOWS_CURL_DIR}/lib/libnghttp2.a"
        "${WINDOWS_CURL_DIR}/lib/libzstd.a"
        "${WINDOWS_CURL_DIR}/lib/libbrotlidec.a"
        "${WINDOWS_CURL_DIR}/lib/libbrotlicommon.a"
        # Windows-specific socket libraries needed by curl
        ws2_32 crypt32 wldap32
    )
    # Copy the curl CA bundle to output directory so HTTPS works at runtime
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${WINDOWS_CURL_DIR}/bin/curl-ca-bundle.crt"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/curl-ca-bundle.crt"
      COMMENT "Copying curl CA bundle to output directory"
    )
    # Set the SSL cert bundle path via compile definition
    add_compile_definitions(CURL_CA_BUNDLE="curl-ca-bundle.crt")
  else()
    message(WARNING "Windows curl bundle not found at ${WINDOWS_CURL_DIR}. AI features disabled.")
  endif()
elseif(CURL_FOUND)
  # Linux / macOS: use system curl
  add_compile_definitions(HAS_CURL=1)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${CURL_LIBRARIES})
  target_include_directories(${PROJECT_NAME} PRIVATE ${CURL_INCLUDE_DIRS})
else()
  message(STATUS "libcurl not found. AI features disabled.")
endif()
# ─────────────────────────────────────────────────────────────────────────────

if(WIN32)
  # winmm is linked transitively by raylib if needed
  # Salin DLL standar MinGW jika melakukan cross-compiling
  if(CMAKE_CROSSCOMPILING AND DEFINED MINGW_PREFIX)
    set(MINGW_BIN_DIR "/usr/${MINGW_PREFIX}/bin")
    foreach(_dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll)
      if(EXISTS "${MINGW_BIN_DIR}/${_dll}")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW_BIN_DIR}/${_dll}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${_dll}"
          COMMENT "Copying MinGW DLL ${_dll} to output directory"
        )
      endif()
    endforeach()
  endif()
endif()


