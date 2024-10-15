#include "vulkan.hpp"

namespace vk
{
	namespace
	{
		rsl::dynamic_library vulkanLibrary;

		constexpr rsl::platform_dependent_var vulkanLibName = {
			rsl::windows_var{"vulkan-1.dll"},
			rsl::linux_var{"libvulkan.so.1"},
		};

		std::vector<extension_properties> availableInstanceExtensions;
		bool libraryIsInitialized = false;

		constexpr semver::version decomposeVkVersion(rsl::uint32 vkVersion)
		{
			return semver::version{
				static_cast<rsl::uint8>(VK_API_VERSION_MAJOR(vkVersion)),
				static_cast<rsl::uint8>(VK_API_VERSION_MINOR(vkVersion)),
				static_cast<rsl::uint8>(VK_API_VERSION_PATCH(vkVersion)),
			};
		}
	} // namespace

#define EXPORTED_VULKAN_FUNCTION(name) PFN_##name name;
#define GLOBAL_LEVEL_VULKAN_FUNCTION(name) PFN_##name name;
#include "impl/list_of_vulkan_functions.inl"

	bool init()
	{
		if (libraryIsInitialized)
		{
			return true;
		}

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

#define GLOBAL_LEVEL_VULKAN_FUNCTION(name)                                                                             \
	name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(nullptr, #name));                                        \
	if (!name)                                                                                                         \
	{                                                                                                                  \
		std::cout << "Could not load global-level Vulkan function \"" #name "\"\n";                                    \
		return false;                                                                                                  \
	}

#include "impl/list_of_vulkan_functions.inl"

		libraryIsInitialized = true;

		return true;
	}

	std::span<const extension_properties> get_available_instance_extensions(bool forceRefresh)
	{
		if (forceRefresh || availableInstanceExtensions.empty())
		{
			rsl::uint32 extensionCount = 0;
			VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
			if (result != VK_SUCCESS || extensionCount == 0)
			{
				std::cout << "Could not query the number of instance extensions.\n";
				return {};
			}

			std::vector<VkExtensionProperties> availableInstanceExtensionsBuffer;
			availableInstanceExtensionsBuffer.resize(extensionCount);
			result = vkEnumerateInstanceExtensionProperties(
				nullptr, &extensionCount, availableInstanceExtensionsBuffer.data()
			);
			if (result != VK_SUCCESS || extensionCount == 0)
			{
				std::cout << "Could not enumerate instance extensions.\n";
				return {};
			}

			availableInstanceExtensions.clear();
			availableInstanceExtensions.reserve(extensionCount);
			for (auto& extension : availableInstanceExtensionsBuffer)
			{
				availableInstanceExtensions.push_back({
					.name = extension.extensionName,
					.specVersion = decomposeVkVersion(extension.specVersion),
				});
			}
		}

		return availableInstanceExtensions;
	}

	bool is_instance_extension_available(std::string_view extensionName)
	{
		for (auto& extension : get_available_instance_extensions())
		{
			if (extension.name == extensionName)
			{
				return true;
			}
		}
		return false;
	}

	instance create_instance(
		const application_info& _applicationInfo, const semver::version& apiVersion, std::span<const char*> extensions
	)
	{
		for (auto& extensionName : extensions)
		{
			if (!is_instance_extension_available(extensionName))
			{
				std::cout << "Extension \"" << extensionName << "\" is not available.\n";
				return {};
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

		vk::instance instance;
		VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance.m_instance);

		if (result != VK_SUCCESS || !instance)
		{
			std::cout << "Failed to create Vulkan Instance\n";
			return {};
		}

		if (!instance.load_functions(extensions))
		{
			std::cout << "Failed to load instance-level functions.\n";
		}

		return instance;
	}

	std::span<physical_device> instance::get_physical_devices(bool forceRefresh)
	{
		if (forceRefresh || m_physicalDevices.empty())
		{
			rsl::uint32 deviceCount = 0;
			VkResult result = vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
			if (result != VK_SUCCESS || deviceCount == 0)
			{
				std::cout << "Could not query the number of physical devices.\n";
				return {};
			}

			std::vector<VkPhysicalDevice> physicalDevicesBuffer;
			physicalDevicesBuffer.resize(deviceCount);
			result = vkEnumeratePhysicalDevices(m_instance, &deviceCount, physicalDevicesBuffer.data());

			if (result != VK_SUCCESS || deviceCount == 0)
			{
				std::cout << "Could not enumerate physical devices.\n";
				return {};
			}

			m_physicalDevices.clear();
			m_physicalDevices.reserve(deviceCount);
			for (auto& pd : physicalDevicesBuffer)
			{
				auto& physicalDevice = m_physicalDevices.emplace_back();
				physicalDevice.m_physicalDevice = pd;

#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION(name) physicalDevice.name = name;
#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION_FROM_EXTENSION(name, extension) physicalDevice.name = name;
#include "impl/list_of_vulkan_functions.inl"
			}
		}

		return m_physicalDevices;
	}

	bool instance::load_functions([[maybe_unused]] std::span<const char*> extensions)
	{
#define INSTANCE_LEVEL_VULKAN_FUNCTION(name)                                                                           \
	name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(m_instance, #name));                                     \
	if (!name)                                                                                                         \
	{                                                                                                                  \
		std::cout << "Could not load instance-level Vulkan function \"" #name "\"\n";                                  \
		return false;                                                                                                  \
	}

#define INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSTION(name, extension)                                                \
	for (auto& enabledExtension : extensions)                                                                          \
	{                                                                                                                  \
		if (std::string_view(enabledExtension) == std::string_view(extension))                                         \
		{                                                                                                              \
			name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(m_instance, #name));                             \
			if (!name)                                                                                                 \
			{                                                                                                          \
				std::cout << "Could not load instance-level Vulkan function \"" #name "\"\n";                          \
				return false;                                                                                          \
			}                                                                                                          \
			break;                                                                                                     \
		}                                                                                                              \
	}

#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION(name)                                                           \
	name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(m_instance, #name));                                     \
	if (!name)                                                                                                         \
	{                                                                                                                  \
		std::cout << "Could not load instance-level Vulkan function \"" #name "\"\n";                                  \
		return false;                                                                                                  \
	}

#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION_FROM_EXTENSION(name, extension)                                 \
	for (auto& enabledExtension : extensions)                                                                          \
	{                                                                                                                  \
		if (std::string_view(enabledExtension) == std::string_view(extension))                                         \
		{                                                                                                              \
			name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(m_instance, #name));                             \
			if (!name)                                                                                                 \
			{                                                                                                          \
				std::cout << "Could not load instance-level Vulkan function \"" #name "\"\n";                          \
				return false;                                                                                          \
			}                                                                                                          \
			break;                                                                                                     \
		}                                                                                                              \
	}

#include "impl/list_of_vulkan_functions.inl"

		return true;
	}

	std::span<const extension_properties> physical_device::get_available_extensions(bool forceRefresh)
	{
		if (forceRefresh || m_availableExtensions.empty())
		{
			rsl::uint32 extensionCount = 0;

			VkResult result = vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, nullptr);
			if (result != VK_SUCCESS || extensionCount == 0)
			{
				std::cout << "Count not query the number of device extensions.\n";
				return {};
			}

			std::vector<VkExtensionProperties> extensionPropertiesBuffer;
			extensionPropertiesBuffer.resize(extensionCount);
			result = vkEnumerateDeviceExtensionProperties(
				m_physicalDevice, nullptr, &extensionCount, extensionPropertiesBuffer.data()
			);

			if (result != VK_SUCCESS || extensionCount == 0)
			{
				std::cout << "Could not enumerate device extensions.\n";
				return {};
			}

			m_availableExtensions.clear();
			m_availableExtensions.reserve(extensionCount);
			for (auto& extension : extensionPropertiesBuffer)
			{
				m_availableExtensions.push_back({
					.name = extension.extensionName,
					.specVersion = decomposeVkVersion(extension.specVersion),
				});
			}
		}

		return m_availableExtensions;
	}

	std::span<const queue_family_properties> physical_device::get_available_queue_families(bool forceRefresh)
	{
		if (forceRefresh || m_availableQueueFamilies.empty())
		{
			rsl::uint32 queueFamilyCount = 0;

			vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);
			if (queueFamilyCount == 0)
			{
				std::cout << "Count not query the number of queue families.\n";
				return {};
			}

			std::vector<VkQueueFamilyProperties> queueFamiliesBuffer;
			queueFamiliesBuffer.resize(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamiliesBuffer.data());

			if (queueFamilyCount == 0)
			{
				std::cout << "Could not get queue family properties.\n";
				return {};
			}

			m_availableQueueFamilies.clear();
			m_availableQueueFamilies.reserve(queueFamilyCount);
			for (auto& queueFamily : queueFamiliesBuffer)
			{
				m_availableQueueFamilies.push_back({
					.features = static_cast<queue_feature_flags>(queueFamily.queueFlags),
					.queueCount = queueFamily.queueCount,
					.timestampValidBits = queueFamily.timestampValidBits,
					.minImageTransferGranularity =
						rsl::math::uint3{
										 queueFamily.minImageTransferGranularity.width,
										 queueFamily.minImageTransferGranularity.height,
										 queueFamily.minImageTransferGranularity.depth,
										 },
				});
			}
		}

		return m_availableQueueFamilies;
	}

	const physical_device_features& physical_device::get_features(bool forceRefresh)
	{
		if (forceRefresh || !m_featuresLoaded)
		{
			vkGetPhysicalDeviceFeatures(m_physicalDevice, &m_features);

			m_featuresLoaded = true;
		}

		return m_features;
	}

	const physical_device_properties& physical_device::get_properties(bool forceRefresh)
	{
		if (forceRefresh || !m_propertiesLoaded)
		{
			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(m_physicalDevice, &props);

			m_properties.apiVersion = decomposeVkVersion(props.apiVersion);
			m_properties.driverVersion = decomposeVkVersion(props.driverVersion);
			m_properties.vendorID = props.vendorID;
			m_properties.deviceID = props.deviceID;
			m_properties.deviceType = props.deviceType;
			m_properties.deviceName = props.deviceName;
			memcpy(m_properties.pipelineCacheUUID, props.pipelineCacheUUID, sizeof(m_properties.pipelineCacheUUID));
			m_properties.limits = props.limits;
			m_properties.sparseProperties = props.sparseProperties;

			m_propertiesLoaded = true;
		}

		return m_properties;
	}

	bool physical_device::initialize(std::span<const char*> extensions)
	{
		return load_functions(extensions);
	}

	bool physical_device::load_functions([[maybe_unused]] std::span<const char*> extensions)
	{
		return false;
	}

	std::string_view to_string(VkPhysicalDeviceType type)
	{
		switch (type)
		{
			case VK_PHYSICAL_DEVICE_TYPE_OTHER: return "Other";
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "Integrated GPU";
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return "Discrete GPU";
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return "Virtual GPU";
			case VK_PHYSICAL_DEVICE_TYPE_CPU: return "CPU";
			case VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM: return "unknown";
		}

		return "unknown";
	}

} // namespace vk
