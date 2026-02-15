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
        ../include/custom
        ../include/custom/sead
        ../include/custom/al

        ../include
        ../include/aarch64
        ../include/agl
        ../include/al
        ../include/eui
        ../include/game
        ../include/sead

        .
)