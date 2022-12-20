C:/Vulkan/Bin/glslc.exe shader.vert -o vert.spv
C:/Vulkan/Bin/glslc.exe shader.frag -o frag.spv
C:/Vulkan/Bin/glslc.exe raytrace.rgen -o rgen.spv --target-env=vulkan1.2
C:/Vulkan/Bin/glslc.exe raytrace.rmiss -o rmiss.spv --target-env=vulkan1.2
C:/Vulkan/Bin/glslc.exe raytrace.rchit -o rhit.spv --target-env=vulkan1.2
pause