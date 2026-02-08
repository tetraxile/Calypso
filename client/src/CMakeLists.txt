target_sources(${PROJECT_NAME}
    PRIVATE
        main.cpp
        menu.cpp
        menuitem.cpp
        server.cpp
        tas.cpp
        util.cpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ../include/custom/sead
        ../include/custom

        ../include
        ../include/aarch64
        ../include/agl
        ../include/al
        ../include/eui
        ../include/game
        ../include/nn
        ../include/sead

        .
)