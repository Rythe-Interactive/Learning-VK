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
		std::cout << '\t' << extension.name << " [" << extension.specVersion << "]\n";
	}

    std::cout << '\n';

	vk::application_info applicationInfo{
		.name = "Learning VK",
		.version = {0, 0, 1},
	};

	vk::instance instance = vk::create_instance(applicationInfo);
	if (!instance)
	{
		return -1;
	}

	for (auto& device : instance.get_physical_devices())
	{
		auto& properties = device.get_properties();

        std::cout << "Device:\n";
		std::cout << "\tapi version: [" << properties.apiVersion << "]\n";
		std::cout << "\tdriver version: [" << properties.driverVersion << "]\n";
		std::cout << "\tvendor ID: " << properties.vendorID << "\n";
		std::cout << "\tdevice ID: " << properties.deviceID << "\n";
		std::cout << "\tdevice type: " << vk::to_string(properties.deviceType) << "\n";
		std::cout << "\tdevice name: \"" << properties.deviceName << "\"\n";

        std::cout << "\tavailable extensions:\n";
		for (auto& extension : device.get_available_extensions())
		{
			std::cout << "\t\t" << extension.name << " [" << extension.specVersion << "]\n ";
		}
	}

	std::cout << "Everything fine so far!\n";
	return 0;
}
