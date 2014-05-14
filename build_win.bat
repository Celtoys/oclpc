
@echo off

REM
REM Copyright 2014 Celtoys Ltd
REM
REM Licensed under the Apache License, Version 2.0 (the "License");
REM you may not use this file except in compliance with the License.
REM You may obtain a copy of the License at
REM
REM     http://www.apache.org/licenses/LICENSE-2.0
REM
REM Unless required by applicable law or agreed to in writing, software
REM distributed under the License is distributed on an "AS IS" BASIS,
REM WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM See the License for the specific language governing permissions and
REM limitations under the License.
REM

setlocal

REM
REM Search for the location of Visual Studio
REM
if DEFINED VS110COMNTOOLS (
	set "VS_TOOLS_DIR=%VS110COMNTOOLS%"
) else if DEFINED VS100COMNTOOLS (
	set "VS_TOOLS_DIR=%VS100COMNTOOLS%"
) else if DEFINED VS90COMNTOOLS (
	set "VS_TOOLS_DIR=%VS90COMNTOOLS%"
) else if DEFINED VS80COMNTOOLS (
	set "VS_TOOLS_DIR=%VS80COMNTOOLS%"
) else (
	echo Microsoft Visual Studio not found
	exit
)


REM
REM Search for the location of OpenCL in all major distributions
REM
if DEFINED CUDA_PATH (
	set "OPENCL_INCLUDE_DIR=%CUDA_PATH%\include"
	set "OPENCL_LIBRARY_DIR=%CUDA_PATH%\lib\Win32"
) else if DEFINED INTELOCLSDKROOT (
	set "OPENCL_INCLUDE_DIR=%INTELOCLSDKROOT%\include"
	set "OPENCL_LIBRARY_DIR=%INTELOCLSDKROOT%\lib\x86"
) else if DEFINED AMDAPPSDKROOT (
	set "OPENCL_INCLUDE_DIR=%AMDAPPSDKROOT%\include"
	set "OPENCL_LIBRARY_DIR=%AMDAPPSDKROOT%\lib\x86"
) else (
	echo OpenCL SDK from nVidia, Intel or AMD not found
	exit
)


REM
REM Apply environment necessary to use cl.exe
REM
set VC_DIR=%VS_TOOLS_DIR%..\..\VC
call "%VC_DIR%\vcvarsall.bat"


REM
REM Quick and dirty compile of the single file
REM
cl.exe src/oclpc.c /nologo /I "%OPENCL_INCLUDE_DIR%" /Fobin/oclpc.obj /link /LIBPATH:"%OPENCL_LIBRARY_DIR%" /OUT:bin/oclpc.exe opencl.lib