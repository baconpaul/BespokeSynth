project(fftw3 VERSION 3.3.10 LANGUAGES C CXX)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "shared" FORCE)
# ON would be nice but there's no SIMDE support so...
# set(ENABLE_SSE2 OFF CACHE BOOL "sse" FORCE)
add_subdirectory(fftw3)

add_library(besfft INTERFACE)
target_link_libraries(besfft INTERFACE fftw3)
target_include_directories(besfft INTERFACE fftw3/api)

add_library(bespoke::${PROJECT_NAME} ALIAS besfft)
