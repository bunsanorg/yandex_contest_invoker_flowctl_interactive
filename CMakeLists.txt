project(yandex_contest_invoker_flowctl_interactive)

include(${CMAKE_SOURCE_DIR}/system-config.cmake OPTIONAL)

cmake_minimum_required(VERSION 2.8)

include(BunsanCommon)

bunsan_use_boost(system filesystem serialization program_options)

bunsan_use_bunsan(common)
bunsan_use(
    yandex_contest_common
    yandex_contest_system
)

bunsan_add_shared_library(${PROJECT_NAME}
    src/lib/Broker.cpp
    src/lib/SimpleBroker.cpp
    src/lib/CommandIo.cpp
)

yandex_contest_add_binary_targets(bin_targets
    simple_broker
)

bunsan_install_headers()
bunsan_install_targets(${PROJECT_NAME} ${bin_targets})

bunsan_include_tests()
