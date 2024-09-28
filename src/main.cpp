#include <iostream>

#include <platform/platform.hpp>
#include <platform/platform_dependent_var.hpp>
#include <rsl/primitives>
#include <rsl/utilities>

#include <vk/vulkan_functions.hpp>

int main()
{
	constexpr rsl::platform_dependent_var vulkanLibName = {
		rsl::windows_var{"vulkan-1.dll"},
		rsl::linux_var{"libvulkan.so.1"},
	};


	rsl::dynamic_library vulkanLibrary = rsl::platform::load_library(vulkanLibName);

	if (!vulkanLibrary)
	{
		std::cout << "could not load " << vulkanLibName.get() << '\n';
		return -1;
	}

	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
		vulkanLibrary.get_symbol<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");

    if (!vkGetInstanceProcAddr)
    {
		std::cout << "could not load vkGetInstanceProcAddr\n";
		return -1;
    }

	std::cout << "Everything fine so far!\n";
	return 0;
}
