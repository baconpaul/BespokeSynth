add_library(r8free STATIC r8brain/r8bbase.cpp r8brain/pffft.cpp)
target_include_directories(r8free PUBLIC r8brain)
target_compile_definitions(r8free PUBLIC R8B_PFFFT=1 R8B_FASTTIMING=1 )

add_library(bespoke::r8brain ALIAS r8free)
