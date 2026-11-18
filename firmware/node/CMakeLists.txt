cmake_minimum_required(VERSION 3.16)

# 共享组件目录（espnow_link: ESP-NOW 封装 / 按键 / LED）
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../components")

include("${CMAKE_CURRENT_LIST_DIR}/../cmake/idf_version_check.cmake")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(node)
