@echo off

REM 编译CPU版本
cl /EHsc cpu_main.cpp particles.cpp /Fe:cpu_demo.exe /link /SUBSYSTEM:WINDOWS opengl32.lib glu32.lib user32.lib gdi32.lib


REM 编译CUDA版本
nvcc cuda_main.cpp particles.cu -o cuda_demo.exe ^
 -lopengl32 -lglu32 -lcudart -luser32 -lgdi32

REM 清理临时文件
@del *.obj *.lib *.exp

