cmake_minimum_required(VERSION 3.10)
project(libcsv)

# ���������, ��� ���������� C99
set(CMAKE_C_STANDARD 99)

# ���� �������� ����� � ������� ����������
file(GLOB CSV_SOURCES "*.c")
file(GLOB CSV_HEADERS "*.h")

if(NOT CSV_SOURCES)
    message(FATAL_ERROR "�� ������� �������� ����� (.c) � ����������: ${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if(NOT CSV_HEADERS)
    message(WARNING "�� ������� ������������ ����� (.h) � ����������: ${CMAKE_CURRENT_SOURCE_DIR}")
endif()

# ������� ���������� (STATIC ��� SHARED)
add_library(csv ${CSV_SOURCES} ${CSV_HEADERS})

# ���������, ��� ������ ���������
target_include_directories(csv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# ��������� (�����������)
install(TARGETS csv DESTINATION lib)
install(FILES ${CSV_HEADERS} DESTINATION include)