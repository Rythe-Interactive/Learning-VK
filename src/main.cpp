#include <iostream>

#include <vk/vulkan.hpp>

int main()
{
	if (!vk::init())
	{
		std::cout << "Failed to initialize vulkan\n";
		return -1;
	}

	std::cout << "Available Instance extensions:\n";
	for (auto& extension : vk::get_available_instance_extensions())
	{
		std::cout << '\t' << extension.extensionName << " [" << extension.specVersion << "]\n";
	}

	vk::application_info applicationInfo{
		.name = "Learning VK",
		.version = {0, 0, 1},
	};

	vk::instance instance = vk::create_instance(applicationInfo);
	if (!instance)
	{
		return -1;
	}

	std::cout << "Everything fine so far!\n";
	return 0;
}
