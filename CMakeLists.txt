project(lab)
set(CMAKE_CXX_STANDARD 17)


set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/install)


set(IMGUI_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends)
set(IMGUI_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui_demo.cpp
                 ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_glfw.cpp
                 ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui.cpp
                 ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui_draw.cpp
                 ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui_widgets.cpp
                 ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui_tables.cpp
                 ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_opengl3.cpp
)
include_directories(${IMGUI_INC_DIR})

# GLM
set(GLM_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm)
include_directories(${GLM_INC_DIR})

# STB
set(STB_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb)
include_directories(${STB_INC_DIR})

# fftw
set(FFTW_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/fftw/)
set(FFTW_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/fftw)
include_directories(${FFTW_INC_DIR})
link_directories(${FFTW_LIB_DIR})

# Glfw
set(GLFW_INSTALL FALSE)
add_subdirectory(vendor/glfw)
include_directories(vendor/glfw/include)

# # Glad
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/vendor/glad/cmake" glad_cmake)
glad_add_library(glad_gl_core_33 STATIC API gl:core=3.3)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/gladsources/glad_gl_core_33/include")
set(GLAD_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/gladsources/glad_gl_core_33/src/gl.c)

# FileDialog
set(FILEDIALOG_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/nfd/src/include)
set(FILEDIALOG_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/vendor/nfd/src/nfd_common.c)
set(FILEDIALOG_SOURCE ${FILEDIALOG_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/vendor/nfd/src/nfd_win.cpp)
include_directories(${FILEDIALOG_INC_DIR})


include_directories(src)

set (sourceFiles
        Main.cpp
        src/Demos/AudioLab/AudioLab.cpp
        src/Demos/ImageLab/ImageLab.cpp
        src/GL_Helpers/GL_Camera.cpp
        src/GL_Helpers/GL_Mesh.cpp
        src/GL_Helpers/GL_Shader.cpp
        src/GL_Helpers/Util.cpp
        ${IMGUI_SOURCE}
        ${GLAD_SOURCE}
        ${FILEDIALOG_SOURCE}
)

add_executable(Lab ${sourceFiles})
target_link_libraries(Lab glfw opengl32 libfftw3-3)
target_link_libraries(Lab ${CUDA_LIBRARIES})

install(TARGETS Lab RUNTIME)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION bin/resources)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shaders/ DESTINATION bin/shaders)
install(FILES ${FFTW_LIB_DIR}/libfftw3-3.dll DESTINATION bin)
