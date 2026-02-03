cmake_minimum_required(VERSION 3.31)
project(SodaDownloader)

set(CMAKE_CXX_STANDARD 20)
find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Iconv REQUIRED)
find_package(taglib REQUIRED)
# --- vcpkg 路径手动指定 (确保路径正确) ---
set(VCPKG_ROOT "D:/Environment/vcpkg/installed/x64-windows")

# 1. 寻找头文件
find_path(LEXBOR_INCLUDE_DIR
        NAMES lexbor/html/html.h
        PATHS "${VCPKG_ROOT}/include"
        NO_DEFAULT_PATH
)

# 2. 寻找库文件 (lexbor.lib)
find_library(LEXBOR_LIBRARY
        NAMES lexbor
        PATHS "${VCPKG_ROOT}/lib"
        NO_DEFAULT_PATH
)

if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_executable(SodaDownloader main.cpp
        src/decryptor.cpp
        src/decryptor.h
        src/stsc_entry.cpp
        src/stsc_entry.h
        src/decrypt_utils.cpp
        src/decrypt_utils.h
        src/mp4_box.cpp
        src/mp4_box.h
        src/utils.cpp
        src/utils.h
        src/media_info.cpp
        src/media_info.h
        src/model_info.cpp
        src/model_info.h
        src/requester.cpp
        src/requester.h
        src/cxxopts.h
        src/lyric_convertor.h
        src/lyric_convertor.cpp
)

target_link_libraries(SodaDownloader PRIVATE CURL::libcurl)
target_link_libraries(SodaDownloader PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(SodaDownloader PRIVATE OpenSSL::SSL)
target_link_libraries(SodaDownloader PRIVATE OpenSSL::Crypto)
target_link_libraries(SodaDownloader PRIVATE Iconv::Iconv)
target_link_libraries(SodaDownloader PRIVATE TagLib::tag TagLib::tag_c TagLib::TagLib)

# 5. 链接库
if(LEXBOR_INCLUDE_DIR AND LEXBOR_LIBRARY)
    message(STATUS "找到 Lexbor 库: ${LEXBOR_LIBRARY}")
    target_include_directories(SodaDownloader PRIVATE ${LEXBOR_INCLUDE_DIR})
    target_link_libraries(SodaDownloader PRIVATE ${LEXBOR_LIBRARY})
else()
    message(FATAL_ERROR "无法在 ${VCPKG_ROOT} 找到 Lexbor！")
endif()
