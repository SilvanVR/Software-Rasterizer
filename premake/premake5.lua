
cwd = "../"

workspace "SoftwareRasterizer"
   location (cwd)
   architecture "x64"
   configurations { "Debug", "Release" }

    
project "SoftwareRasterizer"
   location (cwd)
   kind "ConsoleApp"
   language "C++"
   targetdir (cwd .. "bin/%{cfg.buildcfg}")
   objdir (cwd .. "bin/obj")
   
   files 
   { 
		cwd .. "src/**.h", 
		cwd .. "src/**.cpp" 
   }

   includedirs
   {
   }
   
   filter "configurations:Debug"
      cppdialect "C++17"
	  systemversion "latest"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"