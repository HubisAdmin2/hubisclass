@echo off
echo HubisClass Derleme Scripti
echo ==========================

REM Set PATH to MinGW64 tools
set PATH=%~dp0donanim\mingw64\bin;%PATH%

REM Set directories
set IMGUI_DIR=%~dp0donanim\imgui-1.90.2
set GLFW_INCLUDE=%~dp0donanim\gl\include
set GLFW_LIB=%~dp0donanim\lib
set OUTPUT=%~dp0

echo.
echo [1/9] main.cpp derleniyor...
g++ -std=c++17 -finput-charset=UTF-8 -fexec-charset=UTF-8 -I"%IMGUI_DIR%" -I"%IMGUI_DIR%\backends" -I"%GLFW_INCLUDE%" -c main.cpp -o main.o
if errorlevel 1 goto error

echo [2/9] connect.cpp derleniyor...
g++ -std=c++17 -finput-charset=UTF-8 -fexec-charset=UTF-8 -I"%IMGUI_DIR%" -I"%IMGUI_DIR%\backends" -I"%GLFW_INCLUDE%" -c connect.cpp -o connect.o
if errorlevel 1 goto error

echo [3/9] imgui.cpp derleniyor...
g++ -std=c++17 -finput-charset=UTF-8 -fexec-charset=UTF-8 -I"%IMGUI_DIR%" -I"%IMGUI_DIR%\backends" -I"%GLFW_INCLUDE%" -c "%IMGUI_DIR%\imgui.cpp" -o imgui.o
if errorlevel 1 goto error

echo [4/9] imgui_demo.cpp derleniyor...
g++ -std=c++17 -finput-charset=UTF-8 -fexec-charset=UTF-8 -I"%IMGUI_DIR%" -I"%IMGUI_DIR%\backends" -I"%GLFW_INCLUDE%" -c "%IMGUI_DIR%\imgui_demo.cpp" -o imgui_demo.o
if errorlevel 1 goto error

echo [5/9] imgui_draw.cpp derleniyor...
g++ -std=c++17 -finput-charset=UTF-8 -fexec-charset=UTF-8 -I"%IMGUI_DIR%" -I"%IMGUI_DIR%\backends" -I"%GLFW_INCLUDE%" -c "%IMGUI_DIR%\imgui_draw.cpp" -o imgui_draw.o
if errorlevel 1 goto error

echo [6/9] imgui_tables.cpp derleniyor...
g++ -std=c++17 -finput-charset=UTF-8 -fexec-charset=UTF-8 -I"%IMGUI_DIR%" -I"%IMGUI_DIR%\backends" -I"%GLFW_INCLUDE%" -c "%IMGUI_DIR%\imgui_tables.cpp" -o imgui_tables.o
if errorlevel 1 goto error

echo [7/9] imgui_widgets.cpp derleniyor...
g++ -std=c++17 -finput-charset=UTF-8 -fexec-charset=UTF-8 -I"%IMGUI_DIR%" -I"%IMGUI_DIR%\backends" -I"%GLFW_INCLUDE%" -c "%IMGUI_DIR%\imgui_widgets.cpp" -o imgui_widgets.o
if errorlevel 1 goto error

echo [8/9] imgui_impl_glfw.cpp derleniyor...
g++ -std=c++17 -finput-charset=UTF-8 -fexec-charset=UTF-8 -I"%IMGUI_DIR%" -I"%IMGUI_DIR%\backends" -I"%GLFW_INCLUDE%" -c "%IMGUI_DIR%\backends\imgui_impl_glfw.cpp" -o imgui_impl_glfw.o
if errorlevel 1 goto error

echo [9/9] imgui_impl_opengl3.cpp derleniyor...
g++ -std=c++17 -finput-charset=UTF-8 -fexec-charset=UTF-8 -I"%IMGUI_DIR%" -I"%IMGUI_DIR%\backends" -I"%GLFW_INCLUDE%" -c "%IMGUI_DIR%\backends\imgui_impl_opengl3.cpp" -o imgui_impl_opengl3.o
if errorlevel 1 goto error

echo.
echo Linking...
g++ -std=c++17 -finput-charset=UTF-8 -fexec-charset=UTF-8 main.o connect.o imgui.o imgui_demo.o imgui_draw.o imgui_tables.o imgui_widgets.o imgui_impl_glfw.o imgui_impl_opengl3.o -L"%GLFW_LIB%" -lglfw3 -lopengl32 -lgdi32 -lws2_32 -lwinhttp -static -static-libgcc -static-libstdc++ -o HubisClass.exe
if errorlevel 1 goto error

echo.
echo Temizleniyor...
del main.o connect.o imgui.o imgui_demo.o imgui_draw.o imgui_tables.o imgui_widgets.o imgui_impl_glfw.o imgui_impl_opengl3.o

echo.
echo ==========================
echo Derleme BAŞARILI!
echo HubisClass.exe oluşturuldu.
echo ==========================
echo.
exit /b 0

:error
echo.
echo ==========================
echo HATA: Derleme başarisiz!
echo ==========================
echo.
exit /b 1

