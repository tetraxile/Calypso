set(CMAKE_AUTORCC TRUE)

target_sources(${PROJECT_NAME}
    PRIVATE
        main.cpp
        MainWindow.cpp
        LogWidget.cpp
        util.cpp
        ScriptSTAS.cpp
        BinaryReader.cpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        .
        ../include
)

