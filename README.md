OpenCL Precompiler
------------------

#### Running

This is a very simple OpenCL Precompiler that uses the OpenCL libraries to build kernels you specify on the command-line, reporting any errors as part of your build system. Usage is:

> `oclpc.exe [options] filename`

Command-line options are:

```
   -h                 Display help
   -noheader          Supress header
   -platform_index    Specify zero-based platform index tp select
   -device_index      Specify zero-based device index tp select
   -platform_substr   Specify substring to match when selecting platform by name
   -device_substr     Specify substring to match when selecting device by name
```

Any command-line options that `oclpc` does not recognise are passed on as OpenCL build arguments. Possible build arguments are documented on the Khronos website [here](http://www.khronos.org/registry/cl/sdk/1.0/docs/man/xhtml/clBuildProgram.html).

The binary for Windows in the `bin` directory has been built with MSVC 2010. You will need the [Microsoft Visual C++ 2010 Redistributable Package (x86)][MSVC-Redist] installed to run this (chances are they are already on your system).

#### Building

Run `build_win32.bat` to build for any of the target compilers/SDKS. The source is all in a single C file with no dependencies other than the CRT and OpenCL headers/libraries.

**Builds with**: MSVC 2005 to 2012

**Links with OpenCL from**: [nVidia CUDA Toolkit][CUDA-SDK], [Intel OpenCL SDK][Intel-SDK], [AMD APP SDK][AMD-SDK]

[CUDA-SDK]: https://developer.nvidia.com/cuda-toolkit
[Intel-SDK]: https://software.intel.com/en-us/vcsource/tools/opencl-sdk
[AMD-SDK]: http://developer.amd.com/tools-and-sdks/opencl-zone/opencl-tools-sdks/amd-accelerated-parallel-processing-app-sdk/
[MSVC-Redist]: http://www.microsoft.com/en-us/download/details.aspx?id=5555
