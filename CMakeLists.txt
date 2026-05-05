cmake_minimum_required(VERSION 3.16)

project(visualFrameInfo VERSION 0.1.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 5.12 REQUIRED COMPONENTS Core Gui Widgets Concurrent)

add_executable(visualFrameInfo WIN32
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/models/AnalysisSummary.h
    src/models/FrameInfo.h
    src/models/GopSegment.h
    src/parsers/ShowInfoParser.cpp
    src/parsers/ShowInfoParser.h
    src/services/FfmpegRunner.cpp
    src/services/FfmpegRunner.h
    src/widgets/FrameTableView.cpp
    src/widgets/FrameTableView.h
    src/widgets/GopHistogramWidget.cpp
    src/widgets/GopHistogramWidget.h
    src/widgets/SummaryPanel.cpp
    src/widgets/SummaryPanel.h
    src/widgets/TimelineOverviewBar.cpp
    src/widgets/TimelineOverviewBar.h
    src/widgets/TimelineView.cpp
    src/widgets/TimelineView.h
    src/widgets/PreviewFrameWidget.cpp
    src/widgets/PreviewFrameWidget.h
)

target_link_libraries(visualFrameInfo PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Concurrent
)

if(MSVC)
    target_compile_options(visualFrameInfo PRIVATE /utf-8)
endif()
