
cmake_minimum_required(VERSION 3.16)

project(unicode LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(Threads REQUIRED) # предпочтительнее прямого -lpthread

add_executable(unicode
    main.c
    tables.c
)

target_compile_options(unicode PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    # -Werror
# #    -Wconversion
# #    -Wsign-conversion
#     -Wshadow
#     -Wformat=2
#     -Wundef
#     -Wstrict-prototypes
#     -Wmissing-prototypes
#     -Wold-style-definition
)

target_link_libraries(unicode PRIVATE
    Threads::Threads
)

# libcurl:
# find_package(CURL REQUIRED)
# target_link_libraries(unicode PRIVATE CURL::libcurl)

# SQLite3:
# find_package(SQLite3 REQUIRED)
# target_link_libraries(unicode PRIVATE SQLite::SQLite3)


option(ENABLE_SANITIZERS "Enable AddressSanitizer and UBSan" OFF)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(unicode PRIVATE
        $<$<CONFIG:Debug>:-g;-O0>
        $<$<CONFIG:Release>:-O2;-DNDEBUG>
    )

    if(ENABLE_SANITIZERS)
        target_compile_options(unicode PRIVATE
            -g
            -O1
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
        )

        target_link_options(unicode PRIVATE
            -fsanitize=address,undefined
        )
    endif()
endif()

# сборка
# Debug
# cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
# cmake --build build/debug --parallel
# запуск: ./build/debug/unicode

# Debug + sanitizers
# ASan обнаруживает многие ошибки памяти, UBSan — ряд ошибок неопределённого поведения.
# Оптимизация -O1 облегчает диагностику и сохраняет часть информации о контексте выполнения.
# cmake -S . -B build/sanitize -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
# cmake --build build/sanitize --parallel
# запуск: ./build/sanitize/unicode

# Release
# Оптимизация, определяемая выбранными флагами Release; NDEBUG отключает стандартные проверки assert.
# cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
# cmake --build build/release --parallel
# ./build/release/unicode