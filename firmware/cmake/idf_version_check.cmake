# ============================================================
#  统一 ESP-IDF 版本检查
#
#  在 version.cmake 里改 ESP-IDF 版本时，同步改下面这一行。
#  版本不符会在 CMake 配置阶段直接报错，而不是编到一半冒出莫名其妙的错误。
# ============================================================
set(IDF_VERSION_REQUIRED "5.5.6")

# idf.py 构建时会把 IDF_VERSION 放进环境变量（形如 5.5.6，无 v 前缀）；
# 直接用 cmake 配置的情况则从 version.cmake 读。
if(DEFINED ENV{IDF_VERSION} AND NOT "$ENV{IDF_VERSION}" STREQUAL "")
    set(_idf_ver "$ENV{IDF_VERSION}")
else()
    file(READ "$ENV{IDF_PATH}/tools/cmake/version.cmake" _ver_content)
    string(REGEX MATCH "IDF_VERSION_MAJOR ([0-9]+)" _m "${_ver_content}")
    set(_ver_major "${CMAKE_MATCH_1}")
    string(REGEX MATCH "IDF_VERSION_MINOR ([0-9]+)" _m "${_ver_content}")
    set(_ver_minor "${CMAKE_MATCH_1}")
    string(REGEX MATCH "IDF_VERSION_PATCH ([0-9]+)" _m "${_ver_content}")
    set(_ver_patch "${CMAKE_MATCH_1}")
    set(_idf_ver "${_ver_major}.${_ver_minor}.${_ver_patch}")
endif()

if(NOT _idf_ver STREQUAL IDF_VERSION_REQUIRED)
    message(FATAL_ERROR
        "\n"
        "  ESP-IDF 版本不符\n"
        "  需要: ${IDF_VERSION_REQUIRED}\n"
        "  当前: ${_idf_ver}\n"
        "\n"
        "  请确认 IDF_PATH 指向仓库里的 esp-idf 目录，\n"
        "  或删除 .setup_done 后重新运行 setup.bat。\n")
endif()
