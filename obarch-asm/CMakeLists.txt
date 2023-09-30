# CMakeList.txt : CMake project for oasm, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (obarch-asm "main.cpp" "instruction_decode.cpp")

target_compile_definitions(obarch-asm PUBLIC OASM_OBARCH=1)
set_target_properties(obarch-asm PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET obarch-asm PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
