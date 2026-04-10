set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_PREFIX_PATH /Users/violet/Qt/6.11.0/macos)

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        OpenGL
        OpenGLWidgets
        REQUIRED)

add_executable_auto(GeometryViewer)

# 手动添加 include 目录下的头文件，确保 MOC 能处理带 Q_OBJECT 的头文件
target_sources(GeometryViewer PRIVATE
        ${CMAKE_SOURCE_DIR}/include/app/Layer.h
        ${CMAKE_SOURCE_DIR}/include/app/LayerManagerWidget.h
)

target_include_directories(GeometryViewer PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(GeometryViewer
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGL
        Qt6::OpenGLWidgets
        geometry
        dataengine
)

