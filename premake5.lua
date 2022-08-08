WorkspaceName = "Allocation Tracker"
OutputDirectory = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

workspace (WorkspaceName)
	architecture "x64"
	platforms "x64"
	targetdir (OutputDirectory)
	startproject ("Examples") 

	configurations 
	{ 
		"Debug", 
		"Release",
	}

	flags 
	{
		"MultiProcessorCompile",
	}

	warnings "Extra" -- Warning level

	filter "configurations:Debug"
		defines "CIN_DEBUG" -- Engine define
		symbols "On" -- Specifies whether the compiler should generate debug symbols
		optimize "Off" -- Disables Optimization
		-- optimize "Debug" -- Optimization with some debugger step-through support
		runtime "Debug" -- Specifies the type of runtime library to use
		staticruntime "on" -- Sets runtime library to "MultiThreaded" instead of "MultiThreadedDLL"

	filter "configurations:Release"
		defines "CIN_RELEASE"
		defines "NDEBUG" -- Explicitly define NDEBUG
		symbols "On" 
		optimize "On" 
		runtime "Release"
		staticruntime "on"

project ("AllocationTracker")
	location ("AllocationTracker")
	language "C++"
	cppdialect "C++17"
	kind "Utility"

	targetdir ("bin/" .. (OutputDirectory) .. "/%{prj.name}")
	objdir ("bin-int/" .. (OutputDirectory) .. "/%{prj.name}")

	files 
	{
		"%{prj.name}/include/AllocationTracker/**.h",
	}

project ("Examples")
	location ("Examples")
	language "C++"
	cppdialect "C++17"
	kind "ConsoleApp"

	targetdir ("bin/" .. (OutputDirectory) .. "/%{prj.name}")
	objdir ("bin-int/" .. (OutputDirectory) .. "/%{prj.name}")

	files 
	{ 
		"%{prj.name}/src/Example.cpp",
	}

	includedirs
	{
		"AllocationTracker/include",
	}