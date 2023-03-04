project(fftw3bs VERSION 3.3.10 LANGUAGES CXX)

add_library(${PROJECT_NAME} INTERFACE)
add_subdirectory(fftw3 EXCLUDE_FROM_ALL)

# This means external code says include leathers/blah
target_include_directories(${PROJECT_NAME} INTERFACE .)

add_library(bespoke::${PROJECT_NAME} ALIAS ${PROJECT_NAME})