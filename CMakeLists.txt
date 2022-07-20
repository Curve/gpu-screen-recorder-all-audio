cmake_minimum_required(VERSION 3.1)
project(gpu-screen-recorder-all-audio LANGUAGES CXX VERSION 1.0)

# --------------------------------------------------------------------------------------------------------
# Create library
# --------------------------------------------------------------------------------------------------------

file(GLOB src "src/*.cpp")

add_executable(${PROJECT_NAME} ${src})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Werror -Wextra -pedantic)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)

# --------------------------------------------------------------------------------------------------------
# Link required libraries
# --------------------------------------------------------------------------------------------------------

include(FetchContent)

FetchContent_Declare(rohrkabel GIT_REPOSITORY "https://github.com/Soundux/rohrkabel")
FetchContent_Declare(tiny-process GIT_REPOSITORY "http://gitlab.com/eidheim/tiny-process-library")

FetchContent_MakeAvailable(rohrkabel tiny-process)
target_link_libraries(${PROJECT_NAME} PRIVATE rohrkabel tiny-process-library)