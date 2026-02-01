cmake_minimum_required(VERSION 3.31)
project(SodaDownloader)

set(CMAKE_CXX_STANDARD 20)
find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
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

add_executable(SodaDownloader main.cpp
        include/parser.h
        src/parser.cpp
        include/requester.h
        src/requester.cpp
        include/initializer.h
        src/initializer.cpp
        src/utils.cpp
        include/utils.h
        include/music_info.h
        include/mp4_box.h
        include/box.h
)

target_link_libraries(SodaDownloader PRIVATE CURL::libcurl)
target_link_libraries(SodaDownloader PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(SodaDownloader PRIVATE OpenSSL::SSL)
target_link_libraries(SodaDownloader PRIVATE OpenSSL::Crypto)

# 5. 链接库
if(LEXBOR_INCLUDE_DIR AND LEXBOR_LIBRARY)
    message(STATUS "找到 Lexbor 库: ${LEXBOR_LIBRARY}")
    target_include_directories(SodaDownloader PRIVATE ${LEXBOR_INCLUDE_DIR})
    target_link_libraries(SodaDownloader PRIVATE ${LEXBOR_LIBRARY})
else()
    message(FATAL_ERROR "无法在 ${VCPKG_ROOT} 找到 Lexbor！")
endif()
