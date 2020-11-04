function createProject(vendor)	
	hasCL = findOpenCL(vendor)
	
	if (hasCL) then

		project ("Test_OpenCL_Primitives_" .. vendor)

		initOpenCL(vendor)

		language "C++"
				
		kind "ConsoleApp"
		
		includedirs {".","../../../src"}
		
		
		files {
			"main.cpp",
			"../../../src/Bullet3OpenCL/Initialize/b3OpenCLInclude.h",
			"../../../src/Bullet3OpenCL/Initialize/b3OpenCLUtils.cpp",
			"../../../src/Bullet3OpenCL/Initialize/b3OpenCLUtils.h",
			"../../../src/Bullet3OpenCL/ParallelPrimitives/b3FillCL.cpp",
			"../../../src/Bullet3OpenCL/ParallelPrimitives/b3FillCL.h",
			"../../../src/Bullet3OpenCL/ParallelPrimitives/b3BoundSearchCL.cpp",
			"../../../src/Bullet3OpenCL/ParallelPrimitives/b3BoundSearchCL.h",
			"../../../src/Bullet3OpenCL/ParallelPrimitives/b3PrefixScanCL.cpp",
			"../../../src/Bullet3OpenCL/ParallelPrimitives/b3PrefixScanCL.h",
			"../../../src/Bullet3OpenCL/ParallelPrimitives/b3RadixSort32CL.cpp",
			"../../../src/Bullet3OpenCL/ParallelPrimitives/b3RadixSort32CL.h",
			"../../../src/Bullet3OpenCL/ParallelPrimitives/b3LauncherCL.cpp",
			"../../../src/Bullet3Common/b3AlignedAllocator.cpp",
			"../../../src/Bullet3Common/b3AlignedAllocator.h",
			"../../../src/Bullet3Common/b3AlignedObjectArray.h",
			"../../../src/Bullet3Common/b3Logging.cpp",
			"../../../src/Bullet3Common/b3Logging.h",

		}
		
	end
end

createProject("clew")
createProject("AMD")
createProject("Intel")
createProject("NVIDIA")
createProject("Apple")
