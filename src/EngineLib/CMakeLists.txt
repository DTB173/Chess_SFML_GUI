add_library(EngineLib STATIC)

target_sources(EngineLib
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
            Types.cppm
            Bitboards.cppm
            Position.cppm
            AttackTables.cppm 
            Zobrist.cppm
            PSQTables.cppm
            Magics.cppm
            MoveGen.cppm
)

target_compile_features(EngineLib PUBLIC cxx_std_20)
set_target_properties(EngineLib PROPERTIES CXX_SCAN_FOR_MODULES ON)

if (MSVC)
    target_compile_options(EngineLib PRIVATE /permissive- /W4 /interface) 
else()
    target_compile_options(EngineLib PRIVATE -Wall -Wextra -Wpedantic)
endif()
