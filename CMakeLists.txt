cmake_minimum_required(VERSION 3.10)

project(PGR_cv)
set(SDL_X11_XSCRNSAVER OFF)
add_subdirectory("./sdl")
add_subdirectory("./gegl")
add_subdirectory("./assimp")
add_subdirectory("./glm")

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 geGL::geGL assimp::assimp glm::glm)
