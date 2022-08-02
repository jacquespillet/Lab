@echo off

REM IMGUI
set dearimguiPlot= ..\ext\imgui_plot\
set dearimguiPlotInclude= %dearimguiPlot%\include
REM set dearimguiSrc=%dearimgui%imgui_impl_opengl3.cpp %dearimgui%imgui_impl_glfw.cpp %dearimgui%imgui.cpp %dearimgui%imgui_demo.cpp %dearimgui%imgui_draw.cpp %dearimgui%imgui_widgets.cpp
@REM set dearimguiObj=%dearimgui%\build\imgui_impl_opengl3.obj %dearimgui%\build\imgui_impl_glfw.obj %dearimgui%\build\imgui.obj %dearimgui%\build\imgui_demo.obj %dearimgui%\build\imgui_draw.obj %dearimgui%\build\imgui_widgets.obj
REM --------------------

REM IMGUI
set dearimgui= ..\ext\dearimgui\
set dearimguiSrc=%dearimgui%imgui_impl_opengl3.cpp %dearimgui%imgui_impl_glfw.cpp %dearimgui%imgui.cpp %dearimgui%imgui_demo.cpp %dearimgui%imgui_draw.cpp %dearimgui%imgui_widgets.cpp %dearimgui%imgui_tables.cpp
@REM set dearimguiObj=%dearimgui%\build\imgui_impl_opengl3.obj %dearimgui%\build\imgui_impl_glfw.obj %dearimgui%\build\imgui.obj %dearimgui%\build\imgui_demo.obj %dearimgui%\build\imgui_draw.obj %dearimgui%\build\imgui_widgets.obj
REM --------------------

REM GLEW
set glewLib=..\ext\glew\lib\Release\x64\glew32.lib
set glewIncludes=..\ext\glew\include
set glewBin= C:\Users\jacqu\OneDrive\Documents\Boulot\2020\Exp\ext\glew\bin\Release\x64\glew32.dll
REM --------------------

REM GLFW
set glfwInclude= ..\ext\glfw_\include
set glfwLib= ..\ext\glfw_\lib\glfw3.lib
REM --------------------

REM fftw
set fftwInclude= ..\ext\fftw\include
set fftwLib= ..\ext\fftw\lib\fftw3.lib
set fftwBin= ..\ext\fftw\bin\fftw3.dll
REM --------------------

REM GLM
set glmInclude= ..\ext\glm
REM --------------------

REM ASSIMP
set assimpLibDir= ..\ext\assimp\lib
set assimpLib=..\ext\assimp\lib\assimp-vc141-mt.lib
set assimpIncludes=..\ext\assimp\include
set assimpBin= ..\ext\assimp\bin\assimp-vc141-mt.dll
REM --------------------

REM SRC FILES
set GL_srcFiles= ../ext/imgui_plot/imgui_plot.cpp  ../src/GL_Helpers/GL_Mesh.cpp  ../src/GL_Helpers/GL_Shader.cpp ../src/GL_Helpers/Util.cpp ../src/GL_Helpers/GL_Camera.cpp
set demoFiles= ../src/Demos/ImageLab/ImageLab.cpp ../src/Demos/AudioLab/AudioLab.cpp
set demoFiles= %demoFiles% %dearimguiSrc%

REM --------------------


set compilerFlags= /DEBUG -MP -MT -nologo -EHa- -O2 -Oi -W4 -Z7 -EHsc -wd4201 -wd4310 -wd4100 -wd4312 -wd4996 /I ../src /I ..\ext\stb /I %dearimgui% /I %glfwInclude%  /I %glewIncludes% /I %dearimguiPlotInclude% /I %glmInclude% /I %assimpIncludes% /I %fftwInclude%
set linkerFlags=  -opt:ref Gdi32.lib Shell32.lib opengl32.lib %glfwLib%  %glewLib% %assimpLib% %fftwLib%

IF NOT EXIST .\build mkdir .\build
pushd .\build
cl.exe %compilerFlags% ..\Main.cpp %demoFiles% %GL_srcFiles% /link %linkerFlags%

copy %glewBin% .\ >nul
copy %assimpBin% .\ >nul
copy %fftwBin% .\ >nul

IF NOT EXIST .\shaders mkdir .\shaders
xcopy ..\shaders .\shaders\ /s /e /Y  >nul

IF NOT EXIST .\resources mkdir .\resources
xcopy ..\resources .\resources\  /s /e /Y /D >nul

popd