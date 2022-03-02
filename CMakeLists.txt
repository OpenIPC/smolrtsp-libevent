cmake_minimum_required(VERSION 3.1)
project(smolrtsp-libevent LANGUAGES C)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME}
                           INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_sources(
  ${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src/bufferevent.c
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/evbuffer.c)
