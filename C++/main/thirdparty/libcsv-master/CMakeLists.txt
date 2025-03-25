cmake_minimum_required(VERSION 3.10)
project(libcsv)

# Указываем, что используем C99
set(CMAKE_C_STANDARD 99)

# Ищем исходные файлы в текущей директории
file(GLOB CSV_SOURCES "*.c")
file(GLOB CSV_HEADERS "*.h")

if(NOT CSV_SOURCES)
    message(FATAL_ERROR "Не найдены исходные файлы (.c) в директории: ${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if(NOT CSV_HEADERS)
    message(WARNING "Не найдены заголовочные файлы (.h) в директории: ${CMAKE_CURRENT_SOURCE_DIR}")
endif()

# Создаем библиотеку (STATIC или SHARED)
add_library(csv ${CSV_SOURCES} ${CSV_HEADERS})

# Указываем, где искать заголовки
target_include_directories(csv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Установка (опционально)
install(TARGETS csv DESTINATION lib)
install(FILES ${CSV_HEADERS} DESTINATION include)