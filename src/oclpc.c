
//
// Copyright 2014 Celtoys Ltd
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <assert.h>

#include <CL/cl.h>


// Simple type aliases
typedef unsigned char bool;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef char s8;
typedef short s16;
typedef int s32;


// Define true/false similar to C++
#define true 1
#define false 0


// stdout configuration
bool g_PrintHeader = true;
bool g_PrintHelp = false;
bool g_Verbose = false;

// Selection of platform/device by index from the command-line
s32 g_PlatformIndex = -1;
s32 g_DeviceIndex = -1;

// Selection of platform/device by substring name match
u8 g_PlatformNameSubstr[64] = { 0 };
u8 g_DeviceNameSubstr[64] = { 0 };

char g_InputFilename[256] = { 0 };

char g_BuildArgs[4096] = { 0 };


#define LOG if (g_Verbose) printf

#ifndef _MSC_VER
#include <strings.h>
#define strcmpi strcasecmp
char* strlwr( char* s )
{
	char* p = s;
	while (*p = tolower( *p )) p++;
	return s;
}
#endif

const char* ParseArguments(int argc, const char* argv[])
{
	int i;

	for (i = 1; i < argc; i++)
	{
		const char* arg = argv[i];

		// Is this an option?
		if (arg[0] == '-')
		{
			if (!strcmpi(arg, "-h"))
			{
				g_PrintHelp = true;
			}

			else if (!strcmpi(arg, "-noheader"))
			{
				g_PrintHeader = false;
			}

			else if (!strcmpi(arg, "-verbose"))
			{
				g_Verbose = true;
			}

			else if (!strcmpi(arg, "-platform_index") && i < argc - 1)
			{
				g_PlatformIndex = atoi(argv[i + 1]);
				i++;
			}

			else if (!strcmpi(arg, "-device_index") && i < argc - 1)
			{
				g_DeviceIndex = atoi(argv[i + 1]);
				i++;
			}

			else if (!strcmpi(arg, "-platform_substr") && i < argc - 1)
			{
				strncpy(g_PlatformNameSubstr, argv[i + 1], sizeof(g_PlatformNameSubstr) - 1);
				strlwr(g_PlatformNameSubstr);
				i++;
			}

			else if (!strcmpi(arg, "-device_substr") && i < argc - 1)
			{
				strncpy(g_DeviceNameSubstr, argv[i + 1], sizeof(g_DeviceNameSubstr) - 1);
				strlwr(g_DeviceNameSubstr);
				i++;
			}

			else
			{
				// Add any options that this tool doesn't realise to the build arguments
				strncat(g_BuildArgs, " ", sizeof(g_BuildArgs) - 1);
				strncat(g_BuildArgs, arg, sizeof(g_BuildArgs) - 1);
				if (i < argc - 1 && argv[i + 1][0] != '-')
				{
					strncat(g_BuildArgs, " ", sizeof(g_BuildArgs) - 1);
					strncat(g_BuildArgs, argv[i + 1], sizeof(g_BuildArgs) - 1);
					i++;
				}
			}
		}

		else
		{
			// Must be a filename
			strncpy(g_InputFilename, arg, sizeof(g_InputFilename) - 1);
		}
	}

	if (g_InputFilename[0] == 0)
		return "No input filename specified";

	return NULL;
}


void PrintHeader()
{
	printf("oclpc OpenCL Precompiler Copyright 2014 Celtoys Ltd\n");
	printf("Licensed under the Apache License, Version 2.0 \n");
}


void PrintUsage()
{
	printf("Usage: oclpc [options] filename\n");

	if (g_PrintHelp)
	{
		printf("\nOptions are:\n\n");
		printf("   -noheader          Supress header\n");
		printf("   -verbose           Print logs detailing what oclpc is doing behind the scenes\n");
		printf("   -platform_index    Specify zero-based platform index tp select\n");
		printf("   -device_index      Specify zero-based device index tp select\n");
		printf("   -platform_substr   Specify substring to match when selecting platform by name\n");
		printf("   -device_substr     Specify substring to match when selecting device by name\n");
	}
}


typedef struct
{
	// List of available platforms
	cl_uint nb_platforms;
	cl_platform_id* platform_ids;

	// Selected platform ID
	cl_platform_id platform_id;

	// List of available devices for the selected platform
	cl_uint nb_devices;
	cl_device_id* device_ids;

	// Selected device ID
	cl_device_id device_id;

	cl_context context;
} OpenCL;


const char* OpenCL_GetPlatformIDs(OpenCL* ocl)
{
	cl_int error;

	assert(ocl != NULL);

	// Count number of platforms
	error = clGetPlatformIDs(0, 0, &ocl->nb_platforms);
	if (error != CL_SUCCESS)
		return "clGetPlatformIDs query failed";
	if (ocl->nb_platforms == 0)
		return "No OpenCL platforms found";
	LOG("Number of platforms found: %d\n", ocl->nb_platforms);

	// Get the list of platform IDs
	ocl->platform_ids = malloc(ocl->nb_platforms * sizeof(cl_platform_id));
	if (ocl->platform_ids == NULL)
		return "malloc platform array failed";
	error = clGetPlatformIDs(ocl->nb_platforms, ocl->platform_ids, NULL);
	if (error != CL_SUCCESS)
		return "clGetPlatformIDS call failed";

	return NULL;
}


const char* OpenCL_GetPlatformString(cl_platform_id id, cl_platform_info info, bool to_lower)
{
	static char platform_string[256];
	cl_int error = clGetPlatformInfo(id, info, sizeof(platform_string) - 1, platform_string, NULL);
	if (error != CL_SUCCESS)
		return NULL;

	// Lower-case for substring matching
	if (to_lower)
		strlwr(platform_string);

	return platform_string;
}


const char* OpenCL_SelectPlatform(OpenCL* ocl)
{
	const char* platform_name;

	assert(ocl != NULL);
	assert(ocl->nb_platforms > 0);
	assert(ocl->platform_ids != NULL);

	// Select platform by index provided on command-line first
	if (g_PlatformIndex != -1)
	{
		if (g_PlatformIndex < 0 || g_PlatformIndex >= ocl->nb_platforms)
			return "Out of range platform index provided on command-line";
		ocl->platform_id = ocl->platform_ids[g_PlatformIndex];
		LOG("Platform %d selected by index from command-line\n", g_PlatformIndex);
	}

	else if (g_PlatformNameSubstr[0] != 0)
	{
		// Linear search through platform list for platform with a name matching the input substring
		int i;
		for (i = 0; i < ocl->nb_platforms; i++)
		{
			platform_name = OpenCL_GetPlatformString(ocl->platform_ids[i], CL_PLATFORM_NAME, true);
			if (platform_name == NULL)
				return "GetPlatformString call with CL_PLATFORM_NAME failed";

			if (strstr(platform_name, (const char*)g_PlatformNameSubstr) != NULL)
			{
				ocl->platform_id = ocl->platform_ids[i];
				LOG("Platform %d selected by name matching '%s' from command-line\n", i, g_PlatformNameSubstr);
				break;
			}
		}
	}

	else
	{
		ocl->platform_id = ocl->platform_ids[0];
		LOG("Using platform 0 as no command-line overrides were specified\n");
	}

	// Display the name
	platform_name = OpenCL_GetPlatformString(ocl->platform_id, CL_PLATFORM_NAME, false);
	if (platform_name == NULL)
		return "GetPlatformString call with CL_PLATFORM_NAME failed";
	LOG("Platform '%s' selected with ID %d\n", platform_name, (int)ocl->platform_id);

	return NULL;
}


const char* OpenCL_GetDeviceIDs(OpenCL* ocl)
{
	cl_int error;

	assert(ocl != NULL);
	assert(ocl->platform_id != 0);

	// Count number of devices
	error = clGetDeviceIDs(ocl->platform_id, CL_DEVICE_TYPE_ALL, 0, 0, &ocl->nb_devices);
	if (error != CL_SUCCESS)
		return "clGetDeviceIDs query failed";
	if (ocl->nb_devices == 0)
		return "No OpenCL devices found";
	LOG("Number of devices found: %d\n", ocl->nb_devices);

	// Get the list of device IDs
	ocl->device_ids = malloc(ocl->nb_devices * sizeof(cl_device_id));
	if (ocl->device_ids == NULL)
		return "malloc device array failed";
	error = clGetDeviceIDs(ocl->platform_id, CL_DEVICE_TYPE_ALL, ocl->nb_devices, ocl->device_ids, NULL);
	if (error != CL_SUCCESS)
		return "clGetDeviceIDs call failed";

	return NULL;
}


const char* OpenCL_GetDeviceString(cl_device_id id, cl_device_info info, bool to_lower)
{
	static char device_string[256];
	cl_int error = clGetDeviceInfo(id, info, sizeof(device_string) - 1, device_string, NULL);
	if (error != CL_SUCCESS)
		return NULL;

	// Lower-case for substring matching
	if (to_lower)
		strlwr(device_string);

	return device_string;
}


const char* OpenCL_SelectDevice(OpenCL* ocl)
{
	const char* device_name;

	assert(ocl != NULL);
	assert(ocl->nb_devices > 0);
	assert(ocl->device_ids != NULL);

	// Select device by index provided on command-line first
	if (g_DeviceIndex != -1)
	{
		if (g_DeviceIndex < 0 || g_DeviceIndex >= ocl->nb_devices)
			return "Out of range device index provided on command-line";
		ocl->device_id = ocl->device_ids[g_DeviceIndex];
		LOG("Device %d selected by index from command-line\n", g_DeviceIndex);
	}

	else if (g_DeviceNameSubstr[0] != 0)
	{
		// Linear search through device list for device with a name matching the input substring
		int i;
		for (i = 0; i < ocl->nb_devices; i++)
		{
			device_name = OpenCL_GetDeviceString(ocl->device_ids[i], CL_DEVICE_NAME, true);
			if (device_name == NULL)
				return "GetDeviceString call with CL_DEVICE_NAME failed";

			if (strstr(device_name, (const char*)g_DeviceNameSubstr) != NULL)
			{
				ocl->device_id = ocl->device_ids[i];
				LOG("Device %d selected by name matching '%s' from command-line\n", i, g_DeviceNameSubstr);
				break;
			}
		}
	}

	else
	{
		ocl->device_id = ocl->device_ids[0];
		LOG("Using device 0 as no command-line overrides were specified\n");
	}

	// Display the name
	device_name = OpenCL_GetDeviceString(ocl->device_id, CL_DEVICE_NAME, false);
	if (device_name == NULL)
		return "GetDeviceString call with CL_DEVICE_NAME failed";
	LOG("Device '%s' selected with ID %d\n", device_name, (int)ocl->device_id);

	return NULL;
}


const char* OpenCL_CreateContext(OpenCL* ocl)
{
	cl_int error;
	cl_context_properties props[3];

	assert(ocl != NULL);
	assert(ocl->platform_id != 0);
	assert(ocl->device_id != 0);

	// Specify context platform
	props[0] = CL_CONTEXT_PLATFORM;
	props[1] = (cl_context_properties)ocl->platform_id;
	props[2] = 0;

	// Create the context
	// TODO: Register a callback handler
	ocl->context = clCreateContext(props, 1, &ocl->device_id, 0, 0, &error);
	if (error != CL_SUCCESS)
		return "clCreateContext call failed";

	return NULL;
}


void OpenCL_Destroy(OpenCL** ocl)
{
	assert(ocl != NULL);
	assert(*ocl != NULL);

	if ((*ocl)->context != NULL)
	{
		clReleaseContext((*ocl)->context);
		(*ocl)->context = NULL;
	}

	if ((*ocl)->device_ids != NULL)
	{
		free((*ocl)->device_ids);
		(*ocl)->device_ids = NULL;
	}

	if ((*ocl)->platform_ids != NULL)
	{
		free((*ocl)->platform_ids);
		(*ocl)->platform_ids = NULL;
	}

	free(*ocl);
	*ocl = NULL;
}


const char* OpenCL_Create(OpenCL** ocl)
{
	const char* error_string;

	assert(ocl != NULL);

	// Allocate space for the object
	*ocl = malloc(sizeof(OpenCL));
	if (*ocl == NULL)
		return "malloc failed for sizeof(OpenCL)";

	// Initialise all to defaults
	(*ocl)->nb_platforms = 0;
	(*ocl)->platform_ids = NULL;
	(*ocl)->platform_id = 0;
	(*ocl)->nb_devices = 0;
	(*ocl)->device_ids = NULL;
	(*ocl)->device_id = 0;
	(*ocl)->context = NULL;

	// Select a platform
	if (error_string = OpenCL_GetPlatformIDs(*ocl))
	{
		OpenCL_Destroy(ocl);
		return error_string;
	}
	if (error_string = OpenCL_SelectPlatform(*ocl))
	{
		OpenCL_Destroy(ocl);
		return error_string;
	}

	// Select a device
	if (error_string = OpenCL_GetDeviceIDs(*ocl))
	{
		OpenCL_Destroy(ocl);
		return error_string;
	}
	if (error_string = OpenCL_SelectDevice(*ocl))
	{
		OpenCL_Destroy(ocl);
		return error_string;
	}

	if (error_string = OpenCL_CreateContext(*ocl))
	{
		OpenCL_Destroy(ocl);
		return error_string;
	}

	return NULL;
}


int OpenCL_LoadAndCompileProgram(OpenCL* ocl)
{
	FILE* fp;
	u32 program_size;
	const char* program_data;

	cl_int error;
	cl_program program;

	u32 log_size;
	u8* log_data;

	cl_build_status build_status;

	assert(ocl != NULL);

	// Load the program from disk
	LOG("Opening file %s\n", g_InputFilename);
	fp = fopen(g_InputFilename, "rb");
	if (fp == NULL)
	{
		printf("ERROR: Couldn't open input file %s\n", g_InputFilename);
		return 1;
	}
	fseek(fp, 0, SEEK_END);
	program_size = ftell(fp);
	LOG("File size is %d bytes\n", program_size);
	fseek(fp, 0, SEEK_SET);
	program_data = malloc(program_size);
	if (program_data == NULL)
	{
		printf("ERROR: Failed to allocate memory for program data\n");
		fclose(fp);
		return 1;
	}
	if (fread((void*)program_data, 1, program_size, fp) != program_size)
	{
		printf("ERROR: Couldn't read program data from disk\n");
		free((void*)program_data);
		fclose(fp);
		return 1;
	}
	fclose(fp);

	// Create the program from source and build it
	LOG("Creating and building the program\n");
	program = clCreateProgramWithSource(ocl->context, 1, &program_data, &program_size, &error);
	if (error != CL_SUCCESS)
	{
		free((void*)program_data);
		printf("ERROR: clCreateProgramWithSource call failed\n");
		return 1;
	}
	error = clBuildProgram(program, 1, &ocl->device_id, g_BuildArgs, 0, 0);
	free((void*)program_data);

	// Allocate enough space for the build log and retrieve it
	clGetProgramBuildInfo(program, ocl->device_id, CL_PROGRAM_BUILD_LOG, 0, 0, &log_size);
	LOG("Log size is %d bytes\n", log_size);
	log_data = malloc(log_size);
	if (log_data == NULL)
	{
		printf("ERROR: Failed to allocate log data of size %d\n", log_size);
		return 1;
	}
	clGetProgramBuildInfo(program, ocl->device_id, CL_PROGRAM_BUILD_LOG, log_size, log_data, 0);

	// Query for build success and write an errors/warnings
	error = clGetProgramBuildInfo(program, ocl->device_id, CL_PROGRAM_BUILD_STATUS, sizeof(build_status), &build_status, 0);
	LOG("Build status error code: %d\n", error);
	LOG("Build status code: %d\n", build_status);
	printf("%s", log_data);
	free(log_data);
	return error != CL_SUCCESS || build_status != CL_BUILD_SUCCESS;
}


void AddIncludePathForFile(const char* filename)
{
	// Point to the end of the filename and scan back, looking for the first path separator
	const char* fptr = filename + strlen(filename) - 1;
	while (fptr != filename && *fptr != '/' && *fptr != '\\')
		fptr--;

	// Was a path specified?
	if (fptr != filename)
	{
		u32 path_length = min(fptr - filename, sizeof(g_BuildArgs - 1));

		// Add the path substring as an include
		strncat(g_BuildArgs, " -I \"", sizeof(g_BuildArgs) - 1);
		strncat(g_BuildArgs, filename, path_length);
		strncat(g_BuildArgs, "\"", sizeof(g_BuildArgs) - 1);
	}
}


int main(int argc, const char* argv[])
{
	const char* error;
	int return_code;
	OpenCL* ocl;

	// Attempt to parse arguments
	if (error = ParseArguments(argc, argv))
	{
		PrintHeader();
		printf("\nError parsing arguments: %s\n\n", error);
		PrintUsage();
		return 1;
	}

	// Print program information
	if (g_PrintHeader)
		PrintHeader();
	if (g_PrintHelp)
		PrintUsage();

	// Some OpenCL compilers can't pick up includes in the same directory as the input filename
	// without explicitly telling them about it
	AddIncludePathForFile(g_InputFilename);

	LOG("\n");

	if (error = OpenCL_Create(&ocl))
	{
		printf("\nError initialising OpenCL: %s\n\n", error);
		return 1;
	}

	return_code = OpenCL_LoadAndCompileProgram(ocl);
	OpenCL_Destroy(&ocl);
	return return_code;
}
