#include <vk/vulkan.hpp>

#include <platform/platform.hpp>
#include <platform/platform_dependent_var.hpp>

namespace vk
{
	namespace
	{
		rsl::dynamic_library vulkanLibrary;

		constexpr rsl::platform_dependent_var vulkanLibName = {
			rsl::windows_var{"vulkan-1.dll"},
			rsl::linux_var{"libvulkan.so.1"},
		};
	} // namespace
        
#define EXPORTED_VULKAN_FUNCTION(name) PFN_##name name;
#define GLOBAL_LEVEL_VULKAN_FUNCTION(name) PFN_##name name;
#define INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(name, extension) PFN_##name name;
#define DEVICE_LEVEL_VULKAN_FUNCTION(name) PFN_##name name;
#define DEVICE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(name, extension) PFN_##name name;

#include <vk/impl/list_of_vulkan_functions.inl>

	bool init()
	{
		vulkanLibrary = rsl::platform::load_library(vulkanLibName);

		if (!vulkanLibrary)
		{
			std::cout << "could not load " << vulkanLibName.get() << '\n';
			return false;
		}

#define EXPORTED_VULKAN_FUNCTION(name)                                                                                 \
	name = vulkanLibrary.get_symbol<PFN_##name>(#name);                                                                \
	if (!name)                                                                                                         \
	{                                                                                                                  \
		std::cout << "Could not load exported Vulkan function \"" #name "\"\n";                                        \
		return false;                                                                                                  \
	}

#define GLOBAL_LEVEL_VULKAN_FUNCTION(name) ;

#define INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(name, extension) ;

#define DEVICE_LEVEL_VULKAN_FUNCTION(name) ;

#define DEVICE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(name, extension) ;

#include <vk/impl/list_of_vulkan_functions.inl>

		return true;
	}

} // namespace vk
