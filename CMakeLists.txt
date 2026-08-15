cmake_minimum_required(VERSION 3.24)

project(WinStreamX VERSION 0.1.0 LANGUAGES CXX)

option(WINSTREAMX_BUILD_TESTS "Build WinStreamX tests" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(winstreamx_server
    apps/winstreamx_server/main.cpp
    src/core/version.h
    src/core/result.h
    src/server/server_options.h
    src/server/server_options.cpp
    src/capture/capture_layer.h
    src/capture/capture_layer.cpp
    src/capture/bmp_writer.h
    src/capture/bmp_writer.cpp
    src/capture/captured_frame.h
    src/capture/d3d11_device.h
    src/capture/d3d11_device.cpp
    src/capture/desktop_duplicator.h
    src/capture/desktop_duplicator.cpp
    src/diagnostics/logger.h
    src/diagnostics/logger.cpp
    src/encode/encode_layer.h
    src/encode/encode_layer.cpp
    src/frame/frame_pipeline.h
    src/frame/frame_pipeline.cpp
    src/metrics/metrics_collector.h
    src/metrics/metrics_collector.cpp
    src/server/server_pipeline.h
    src/server/server_pipeline.cpp
    src/transport/transport_layer.h
    src/transport/transport_layer.cpp
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES
    apps/winstreamx_server/main.cpp
    src/core/version.h
    src/core/result.h
    src/server/server_options.h
    src/server/server_options.cpp
    src/capture/capture_layer.h
    src/capture/capture_layer.cpp
    src/capture/bmp_writer.h
    src/capture/bmp_writer.cpp
    src/capture/captured_frame.h
    src/capture/d3d11_device.h
    src/capture/d3d11_device.cpp
    src/capture/desktop_duplicator.h
    src/capture/desktop_duplicator.cpp
    src/diagnostics/logger.h
    src/diagnostics/logger.cpp
    src/encode/encode_layer.h
    src/encode/encode_layer.cpp
    src/frame/frame_pipeline.h
    src/frame/frame_pipeline.cpp
    src/metrics/metrics_collector.h
    src/metrics/metrics_collector.cpp
    src/server/server_pipeline.h
    src/server/server_pipeline.cpp
    src/transport/transport_layer.h
    src/transport/transport_layer.cpp
)

target_include_directories(winstreamx_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(winstreamx_server PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

if (MSVC)
    target_compile_options(winstreamx_server PRIVATE /W4 /permissive-)
endif()

target_link_libraries(winstreamx_server PRIVATE
    d3d11
    dxgi
)

enable_testing()

if (WINSTREAMX_BUILD_TESTS)
    add_executable(winstreamx_server_tests
        tests/server_options_tests.cpp
        src/server/server_options.cpp
    )
    target_include_directories(winstreamx_server_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    if (MSVC)
        target_compile_options(winstreamx_server_tests PRIVATE /W4 /permissive-)
    endif()
    add_test(NAME winstreamx_server_tests COMMAND winstreamx_server_tests)

    add_executable(winstreamx_capture_tests
        tests/bmp_writer_tests.cpp
        src/capture/bmp_writer.cpp
    )
    target_include_directories(winstreamx_capture_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    if (MSVC)
        target_compile_options(winstreamx_capture_tests PRIVATE /W4 /permissive-)
    endif()
    add_test(NAME winstreamx_capture_tests COMMAND winstreamx_capture_tests)
endif()
