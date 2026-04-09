# 内部函数：收集源码并加入 target
function(_collect_and_add_sources TARGET_NAME)
    set(CURRENT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

    file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS
            ${CURRENT_DIR}/*.h
            ${CURRENT_DIR}/*.hpp
    )

    file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
            ${CURRENT_DIR}/*.cpp
    )

    target_sources(${TARGET_NAME}
            PRIVATE
            ${HEADER_FILES}
            ${SOURCE_FILES}
    )

    # IDE 分组（VS / CLion 很有用）
    source_group(TREE ${CURRENT_DIR} FILES
            ${HEADER_FILES}
            ${SOURCE_FILES}
    )
endfunction()

function(add_shared_lib_auto TARGET_NAME)
    add_library(${TARGET_NAME} SHARED)
    _collect_and_add_sources(${TARGET_NAME})
endfunction()

function(add_static_lib_auto TARGET_NAME)
    add_library(${TARGET_NAME} STATIC)
    _collect_and_add_sources(${TARGET_NAME})
endfunction()

function(add_executable_auto TARGET_NAME)
    add_executable(${TARGET_NAME})
    _collect_and_add_sources(${TARGET_NAME})
endfunction()