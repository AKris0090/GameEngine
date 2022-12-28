C:/VulkanSDK/1.3.236.0/Bin/glslc.exe shader.vert -o vert.spv
C:/VulkanSDK/1.3.236.0/Bin/glslc.exe shader.frag -o frag.spv
C:/VulkanSDK/1.3.236.0/Bin/glslc.exe raytrace.rgen -o rgen.spv --target-env=vulkan1.2
C:/VulkanSDK/1.3.236.0/Bin/glslc.exe raytrace.rmiss -o rmiss.spv --target-env=vulkan1.2
C:/VulkanSDK/1.3.236.0/Bin/glslc.exe raytrace.rchit -o rchit.spv --target-env=vulkan1.2
pause