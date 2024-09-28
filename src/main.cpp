#include <iostream>

#include <vk/vulkan.hpp>

int main()
{
    if (!vk::init())
    {
		std::cout << "Failed to initialize vulkan\n";
		return -1;
    }

    auto procAddr = &vk::vkGetInstanceProcAddr;

	std::cout << procAddr << '\n';

	std::cout << "Everything fine so far!\n";
	return 0;
}
