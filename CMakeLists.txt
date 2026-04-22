cmake_minimum_required(VERSION 3.28)
project(Chess_GUI LANGUAGES CXX)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.0 
)

set(SFML_BUILD_AUDIO TRUE)
set(SFML_BUILD_NETWORK FALSE)
FetchContent_MakeAvailable(SFML)

add_subdirectory(src/EngineLib)

add_executable(main WIN32 src/main.cpp)

target_link_libraries(main 
    PRIVATE 
        EngineLib
        sfml-graphics
        sfml-audio
        sfml-window
        sfml-system
        sfml-main
)

target_sources(main
    PRIVATE
        FILE_SET CXX_MODULES FILES
            src/WindowManager.cppm
            src/Button.cppm
            src/GameState.cppm
            src/EngineInterface.cppm
            src/PlayerCard.cppm
            src/MovePanel.cppm
)

target_compile_features(main PRIVATE cxx_std_20)

add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:main>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/chess_engine.exe" 
        "$<TARGET_FILE_DIR:main>/chess_engine.exe" 
    COMMENT "Deploying Assets and Engine to build folder..."
)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT main)