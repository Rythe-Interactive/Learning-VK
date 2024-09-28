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

		std::vector<VkExtensionProperties> availableInstanceExtensionsBuffer;
		std::vector<extension_properties> availableInstanceExtensions;
	} // namespace

#define EXPORTED_VULKAN_FUNCTION(name) PFN_vk##name name;
#define GLOBAL_LEVEL_VULKAN_FUNCTION(name) PFN_vk##name name;
#define INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(name, extension) PFN_vk##name name;
#define DEVICE_LEVEL_VULKAN_FUNCTION(name) PFN_vk##name name;
#define DEVICE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(name, extension) PFN_vk##name name;

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
	name = vulkanLibrary.get_symbol<PFN_vk##name>("vk" #name);                                                         \
	if (!name)                                                                                                         \
	{                                                                                                                  \
		std::cout << "Could not load exported Vulkan function \"vk" #name "\"\n";                                      \
		return false;                                                                                                  \
	}

#define GLOBAL_LEVEL_VULKAN_FUNCTION(name)                                                                             \
	name = reinterpret_cast<PFN_vk##name>(GetInstanceProcAddr(nullptr, "vk" #name));                                   \
	if (!name)                                                                                                         \
	{                                                                                                                  \
		std::cout << "Could not load global-level Vulkan function \"vk" #name "\"\n";                                  \
		return false;                                                                                                  \
	}

#define INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(name, extension) ;

#define DEVICE_LEVEL_VULKAN_FUNCTION(name) ;

#define DEVICE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(name, extension) ;

#include <vk/impl/list_of_vulkan_functions.inl>

		rsl::uint32 extensionCount = 0;
		VkResult result = EnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		if (result != VK_SUCCESS || extensionCount == 0)
		{
			std::cout << "Could not query the number of Instance extensions.\n";
			return false;
		}

		availableInstanceExtensionsBuffer.resize(extensionCount);
		result =
			EnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableInstanceExtensionsBuffer.data());
		if (result != VK_SUCCESS || extensionCount == 0)
		{
			std::cout << "Could not enumerate Instance extensions.\n";
			return false;
		}

		availableInstanceExtensions.reserve(extensionCount);
		for (auto& extension : availableInstanceExtensionsBuffer)
		{
			availableInstanceExtensions.push_back({
				.extensionName = extension.extensionName,
				.specVersion =
					{
						static_cast<rsl::uint8>(VK_API_VERSION_MAJOR(extension.specVersion)),
						static_cast<rsl::uint8>(VK_API_VERSION_MINOR(extension.specVersion)),
						static_cast<rsl::uint8>(VK_API_VERSION_PATCH(extension.specVersion)),
					},
			});
		}

		return true;
	}

	std::span<extension_properties> get_available_instance_extensions()
	{
		return availableInstanceExtensions;
	}

	bool is_instance_extension_available(std::string_view extensionName)
	{
		for (auto& extension : availableInstanceExtensions)
		{
			if (extension.extensionName == extensionName)
			{
				return true;
			}
		}
		return false;
	}

	instance create_instance(
		const application_info& _applicationInfo, const semver::version& apiVersion,
		std::span<const char*> extensions
	)
	{
		vk::instance instance;

		for (auto& extensionName : extensions)
		{
			if (!is_instance_extension_available(extensionName))
			{
				std::cout << "Extension \"" << extensionName << "\" is not available.\n";
				return instance;
			}
		}

		VkApplicationInfo applicationInfo{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = nullptr,
			.pApplicationName = _applicationInfo.name.c_str(),
			.applicationVersion = VK_MAKE_API_VERSION(
				0, _applicationInfo.version.major, _applicationInfo.version.minor, _applicationInfo.version.patch
			),
			.pEngineName = "Rythe Engine",
			.engineVersion = VK_MAKE_API_VERSION(0, 0, 0, 1),
			.apiVersion = VK_MAKE_API_VERSION(0, apiVersion.major, apiVersion.minor, apiVersion.patch),
		};

        VkInstanceCreateInfo instanceCreateInfo{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pApplicationInfo = &applicationInfo,
            .enabledLayerCount = 0,
			.ppEnabledLayerNames = nullptr,
			.enabledExtensionCount = static_cast<rsl::uint32>(extensions.size()),
			.ppEnabledExtensionNames = extensions.data(),
		};

        VkResult result = CreateInstance(&instanceCreateInfo, nullptr, &instance._instance);

        if (result != VK_SUCCESS || !instance)
        {
			std::cout << "Failed to create Vulkan Instance\n";
			instance = {};
        }

		return instance;
	}

} // namespace vk
