cmake_minimum_required(VERSION 3.0)

include(${CMAKE_SOURCE_DIR}/system-config.cmake OPTIONAL)

project(yandex_contest_invoker_flowctl_interactive)

if(BunsanCMakeBuiltin)
    BunsanCMakeLoad()
else()
    find_package(BunsanCMake CONFIG REQUIRED)
endif()

bunsan_add_shared_library(${PROJECT_NAME}
    src/lib/Broker.cpp
    src/lib/BufferedConnection.cpp
    src/lib/SimpleBroker.cpp
    src/lib/CommandIo.cpp
)
bunsan_use_bunsan_package(${PROJECT_NAME} yandex_contest_invoker yandex_contest_invoker)

bunsan_add_executable(${PROJECT_NAME}_simple_broker
    src/bin/simple_broker.cpp
)
bunsan_use_target(${PROJECT_NAME}_simple_broker ${PROJECT_NAME})

bunsan_install_headers()
bunsan_install_targets(${PROJECT_NAME} ${bin_targets})
bunsan_install_project()

bunsan_include_tests()
