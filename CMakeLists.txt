cmake_minimum_required(VERSION 3.10.0)
project(smolrtsp-libevent LANGUAGES C)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME}
                           INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_sources(
  ${PROJECT_NAME}
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src/bufferevent.c
            ${CMAKE_CURRENT_SOURCE_DIR}/src/evbuffer.c
            ${CMAKE_CURRENT_SOURCE_DIR}/src/dispatcher.c)
