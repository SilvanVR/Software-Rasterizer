
root = "../"
cwd = root .. "build/"

workspace "SoftwareRasterizer"
   location (cwd)
   architecture "x64"
   configurations { "Debug", "Release" }

libs = "/libs"
    
project "SoftwareRasterizer"
   location (cwd)
   kind "ConsoleApp"
   language "C++"
   targetdir (root .. "bin/%{cfg.buildcfg}")
   objdir (root .. "bin/obj")
   
   pchheader "stdafx.h"
   pchsource (cwd .. "src/stdafx.cpp")
   
   files 
   { 
		cwd .. "src/**.h", 
		cwd .. "src/**.hpp", 
		cwd .. "src/**.cpp" 
   }

   includedirs
   {  
      cwd .. "src"
   }
   
   filter "system:windows"
      cppdialect "C++17"
	  systemversion "latest"
      defines { "PLATFORM_WIN32" }
		
   filter "configurations:Debug"	  
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"