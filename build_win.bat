
@echo off

::
:: Copyright 2014 Celtoys Ltd
::
:: Licensed under the Apache License, Version 2.0 (the "License");
:: you may not use this file except in compliance with the License.
:: You may obtain a copy of the License at
::
::     http://www.apache.org/licenses/LICENSE-2.0
::
:: Unless required by applicable law or agreed to in writing, software
:: distributed under the License is distributed on an "AS IS" BASIS,
:: WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
:: See the License for the specific language governing permissions and
:: limitations under the License.
::

setlocal

::
:: Search for the location of Visual Studio
::
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


::
:: Search for the location of OpenCL in all major distributions
::
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


::
:: Apply environment necessary to use cl.exe
::
set VC_DIR=%VS_TOOLS_DIR%..\..\VC
call "%VC_DIR%\vcvarsall.bat"


::
:: Search for the windows SDK
::
set KEY_NAME="HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows"
set VALUE_NAME=CurrentInstallFolder
for /F "usebackq skip=2 tokens=1,2*" %%A in (`REG QUERY %KEY_NAME% /v %VALUE_NAME% 2^>nul`) do (
	set "ValueName=%%A"
	set "ValueType=%%B"
	set WINDOWS_SDK_DIR=%%C
)
if not defined WINDOWS_SDK_DIR (
	echo %KEY_NAME%\%VALUE_NAME% not found.
	exit
)


::
:: Quick and dirty compile of the single file
::
cl.exe src/oclpc.c /nologo /I "%OPENCL_INCLUDE_DIR%" /Fobin/oclpc.obj /link /LIBPATH:"%OPENCL_LIBRARY_DIR%" /LIBPATH:"%WINDOWS_SDK_DIR%lib" /OUT:bin/oclpc.exe opencl.lib