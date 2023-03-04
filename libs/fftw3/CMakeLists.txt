project(fftw3bs VERSION 3.3.10 LANGUAGES C CXX)

add_library(${PROJECT_NAME} fftw3)
add_subdirectory(fftw3)

# This means external code says include fftw3/blah
target_include_directories(${PROJECT_NAME} PUBLIC .)
target_include_directories(${PROJECT_NAME} PRIVATE fftw3)

add_library(bespoke::${PROJECT_NAME} ALIAS ${PROJECT_NAME})