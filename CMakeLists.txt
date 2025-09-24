cmake_minimum_required(VERSION 3.16)

# Naziv i verzija projekta (možeš prilagoditi)
project(MyProject
    VERSION 1.0.0
    LANGUAGES C CXX
)

# Opcije za build
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ako želiš da svi targeti koriste warnings
if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Dodaj poddirektorijume
add_subdirectory(software)

# Ako imaš i druge foldere (npr. libs, tests, docs, cmake skripte...)
# add_subdirectory(libs)
# add_subdirectory(tests)

