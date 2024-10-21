#include "vulkan.hpp"

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#if RYTHE_PLATFORM_WINDOWS
	#define VK_USE_PLATFORM_WIN32_KHR
	#define WIN32_LEAN_AND_MEAN
	#define VC_EXTRALEAN
	#define NOMINMAX
	#include <windef.h>
	#include <minwinbase.h>
	#include <vulkan/vulkan_win32.h>
#elif RYTHE_PLATFORM_LINUX
	#ifdef RYTHE_SURFACE_XCB
		#define VK_USE_PLATFORM_XCB_KHR
		#include <vulkan/vulkan_xcb.h>
	#endif
	#ifdef RYTHE_SURFACE_XLIB
		#define VK_USE_PLATFORM_XLIB_KHR
		#include <vulkan/vulkan_xlib.h>
	#endif
#endif

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

		rythe_always_inline constexpr semver::version decomposeVkVersion(rsl::uint32 vkVersion)
		{
			return semver::version{
				static_cast<rsl::uint8>(VK_API_VERSION_MAJOR(vkVersion)),
				static_cast<rsl::uint8>(VK_API_VERSION_MINOR(vkVersion)),
				static_cast<rsl::uint8>(VK_API_VERSION_PATCH(vkVersion)),
			};
		}

#define EXPORTED_VULKAN_FUNCTION(name) PFN_##name name = nullptr;
#define GLOBAL_LEVEL_VULKAN_FUNCTION(name) PFN_##name name = nullptr;
#include "impl/list_of_vulkan_functions.inl"

	} // namespace

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

	class native_instance_vk
	{
	public:
		bool load_functions(std::span<rsl::cstring> extensions);

#define INSTANCE_LEVEL_VULKAN_FUNCTION(name) [[maybe_unused]] PFN_##name name = nullptr;
#define INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(name, extension) [[maybe_unused]] PFN_##name name = nullptr;
#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION(name) [[maybe_unused]] PFN_##name name = nullptr;
#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION_FROM_EXTENSION(name, extension)                                 \
	[[maybe_unused]] PFN_##name name = nullptr;
#define INSTANCE_LEVEL_DEVICE_VULKAN_FUNCTION(name) [[maybe_unused]] PFN_##name name = nullptr;
#include "impl/list_of_vulkan_functions.inl"

		std::vector<physical_device> physicalDevices;

		VkInstance instance = VK_NULL_HANDLE;
	};

	template <typename T>
	struct native_handle_traits
	{
	};

	template <>
	struct native_handle_traits<vk::instance>
	{
		using native_type = native_instance_vk;
		using handle_type = native_instance;
		constexpr static handle_type invalid_handle = invalid_native_instance;
	};

	template <>
	struct native_handle_traits<native_instance_vk>
	{
		using api_type = vk::instance;
		using handle_type = native_instance;
		constexpr static handle_type invalid_handle = invalid_native_instance;
	};

	class native_physical_device_vk
	{
	public:
		bool load_functions(std::span<rsl::cstring> extensions);

#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION(name) [[maybe_unused]] PFN_##name name = nullptr;
#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION_FROM_EXTENSION(name, extension)                                 \
	[[maybe_unused]] PFN_##name name = nullptr;
#include "impl/list_of_vulkan_functions.inl"

		render_device renderDevice;

		bool featuresLoaded = false;
		physical_device_features features;
		bool propertiesLoaded = false;
		physical_device_properties properties;
		std::vector<extension_properties> availableExtensions;
		std::vector<queue_family_properties> availableQueueFamilies;

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	};

	template <>
	struct native_handle_traits<physical_device>
	{
		using native_type = native_physical_device_vk;
		using handle_type = native_physical_device;
		constexpr static handle_type invalid_handle = invalid_native_physical_device;
	};

	template <>
	struct native_handle_traits<native_physical_device_vk>
	{
		using api_type = physical_device;
		using handle_type = native_physical_device;
		constexpr static handle_type invalid_handle = invalid_native_physical_device;
	};

	class native_render_device_vk
	{
	public:
		VkDevice device = VK_NULL_HANDLE;
	};

	template <>
	struct native_handle_traits<render_device>
	{
		using native_type = native_render_device_vk;
		using handle_type = native_render_device;
		constexpr static handle_type invalid_handle = invalid_native_render_device;
	};

	template <>
	struct native_handle_traits<native_render_device_vk>
	{
		using api_type = render_device;
		using handle_type = native_render_device;
		constexpr static handle_type invalid_handle = invalid_native_render_device;
	};

	namespace
	{
		template <typename T>
		rythe_always_inline typename native_handle_traits<T>::native_type* get_native_ptr(const T& inst)
		{
			using native_type = typename native_handle_traits<T>::native_type;
			using handle_type = typename native_handle_traits<T>::handle_type;

			handle_type handle = inst.get_native_handle();

			if (handle == native_handle_traits<T>::invalid_handle)
			{
				return nullptr;
			}

			native_type* ptr = nullptr;
			memcpy(&ptr, &handle, sizeof(rsl::ptr_type));

			return ptr;
		}

		template <typename T>
		rythe_always_inline auto create_native_handle(T* inst)
		{
			using handle_type = typename native_handle_traits<T>::handle_type;

			T* ptr = inst;
			handle_type handle = handle_type::invalid;
			memcpy(&handle, &ptr, sizeof(rsl::ptr_type));

			return handle;
		}
	} // namespace

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
		native_instance_vk* nativeInstance = new native_instance_vk();
		instance.m_nativeInstance = create_native_handle(nativeInstance);

		VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &nativeInstance->instance);

		if (result != VK_SUCCESS || !instance)
		{
			std::cout << "Failed to create Vulkan Instance\n";
			return {};
		}

		if (!nativeInstance->load_functions(extensions))
		{
			std::cout << "Failed to load instance-level functions.\n";
		}

		return instance;
	}

	std::string_view to_string(physical_device_type type)
	{
		switch (type)
		{
			case physical_device_type::Other: return "Other";
			case physical_device_type::Integrated: return "Integrated GPU";
			case physical_device_type::Discrete: return "Discrete GPU";
			case physical_device_type::Virtual: return "Virtual GPU";
			case physical_device_type::CPU: return "CPU";
		}

		return "unknown";
	}

	std::string_view to_string(queue_priority priority)
	{
		switch (priority)
		{
			case queue_priority::Normal: return "Normal";
			case queue_priority::High: return "High";
		}

		return "unknown";
	}

	instance::operator bool() const noexcept
	{
		auto ptr = get_native_ptr(*this);
		return ptr != nullptr && ptr->instance != VK_NULL_HANDLE;
	}

	std::span<physical_device> instance::create_physical_devices(bool forceRefresh)
	{
		auto* impl = get_native_ptr(*this);

		if (forceRefresh || impl->physicalDevices.empty())
		{
			rsl::uint32 deviceCount = 0;
			VkResult result = impl->vkEnumeratePhysicalDevices(impl->instance, &deviceCount, nullptr);
			if (result != VK_SUCCESS || deviceCount == 0)
			{
				std::cout << "Could not query the number of physical devices.\n";
				return {};
			}

			std::vector<VkPhysicalDevice> physicalDevicesBuffer;
			physicalDevicesBuffer.resize(deviceCount);
			result = impl->vkEnumeratePhysicalDevices(impl->instance, &deviceCount, physicalDevicesBuffer.data());

			if (result != VK_SUCCESS || deviceCount == 0)
			{
				std::cout << "Could not enumerate physical devices.\n";
				return {};
			}

			force_release_all_physical_devices();
			impl->physicalDevices.reserve(deviceCount);
			for (auto& pd : physicalDevicesBuffer)
			{
				auto& physicalDevice = impl->physicalDevices.emplace_back();
				native_physical_device_vk* nativePhysicalDevice = new native_physical_device_vk();
				physicalDevice.m_nativePhysicalDevice = create_native_handle(nativePhysicalDevice);

				nativePhysicalDevice->physicalDevice = pd;

#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION(name) nativePhysicalDevice->name = impl->name;
#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION_FROM_EXTENSION(name, extension)                                 \
	nativePhysicalDevice->name = impl->name;
#include "impl/list_of_vulkan_functions.inl"
			}
		}

		return impl->physicalDevices;
	}

	void instance::release_unused_physical_devices()
	{
		auto* impl = get_native_ptr(*this);

        std::vector<physical_device> newDeviceList;

		for (auto& device : impl->physicalDevices)
		{
			auto ptr = get_native_ptr(device); 
			if (ptr != nullptr)
			{
                if (device.in_use())
                {
					newDeviceList.push_back(device);
                }
				else
				{
					delete ptr;
				}
			}
		}

		impl->physicalDevices = std::move(newDeviceList);
	}

	void instance::force_release_all_physical_devices()
	{
		auto* impl = get_native_ptr(*this);

		for (auto& device : impl->physicalDevices)
		{
			if (auto ptr = get_native_ptr(device); ptr != nullptr)
			{
				delete ptr;
			}
		}

        impl->physicalDevices.clear();
	}

	render_device instance::auto_select_and_create_device(
		const physical_device_description& physicalDeviceDescription,
		std::span<const queue_description> queueDesciptions
	)
	{
		return render_device();
	}

	bool native_instance_vk::load_functions([[maybe_unused]] std::span<const char*> extensions)
	{
#define INSTANCE_LEVEL_VULKAN_FUNCTION(name)                                                                           \
	name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name));                                       \
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
			name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name));                               \
			if (!name)                                                                                                 \
			{                                                                                                          \
				std::cout << "Could not load instance-level Vulkan function \"" #name "\"\n";                          \
				return false;                                                                                          \
			}                                                                                                          \
			break;                                                                                                     \
		}                                                                                                              \
	}

#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION(name)                                                           \
	name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name));                                       \
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
			name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name));                               \
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
		auto* impl = get_native_ptr(*this);

		if (forceRefresh || impl->availableExtensions.empty())
		{
			rsl::uint32 extensionCount = 0;

			VkResult result =
				impl->vkEnumerateDeviceExtensionProperties(impl->physicalDevice, nullptr, &extensionCount, nullptr);
			if (result != VK_SUCCESS || extensionCount == 0)
			{
				std::cout << "Count not query the number of device extensions.\n";
				return {};
			}

			std::vector<VkExtensionProperties> extensionPropertiesBuffer;
			extensionPropertiesBuffer.resize(extensionCount);
			result = impl->vkEnumerateDeviceExtensionProperties(
				impl->physicalDevice, nullptr, &extensionCount, extensionPropertiesBuffer.data()
			);

			if (result != VK_SUCCESS || extensionCount == 0)
			{
				std::cout << "Could not enumerate device extensions.\n";
				return {};
			}

			impl->availableExtensions.clear();
			impl->availableExtensions.reserve(extensionCount);
			for (auto& extension : extensionPropertiesBuffer)
			{
				impl->availableExtensions.push_back({
					.name = extension.extensionName,
					.specVersion = decomposeVkVersion(extension.specVersion),
				});
			}
		}

		return impl->availableExtensions;
	}

	std::span<const queue_family_properties> physical_device::get_available_queue_families(bool forceRefresh)
	{
		auto* impl = get_native_ptr(*this);

		if (forceRefresh || impl->availableQueueFamilies.empty())
		{
			rsl::uint32 queueFamilyCount = 0;

			impl->vkGetPhysicalDeviceQueueFamilyProperties(impl->physicalDevice, &queueFamilyCount, nullptr);
			if (queueFamilyCount == 0)
			{
				std::cout << "Count not query the number of queue families.\n";
				return {};
			}

			std::vector<VkQueueFamilyProperties> queueFamiliesBuffer;
			queueFamiliesBuffer.resize(queueFamilyCount);
			impl->vkGetPhysicalDeviceQueueFamilyProperties(
				impl->physicalDevice, &queueFamilyCount, queueFamiliesBuffer.data()
			);

			if (queueFamilyCount == 0)
			{
				std::cout << "Could not get queue family properties.\n";
				return {};
			}

			impl->availableQueueFamilies.clear();
			impl->availableQueueFamilies.reserve(queueFamilyCount);
			for (auto& queueFamily : queueFamiliesBuffer)
			{
				impl->availableQueueFamilies.push_back({
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

		return impl->availableQueueFamilies;
	}

	namespace
	{
		void map_vk_physical_device_features(physical_device_features& target, const VkPhysicalDeviceFeatures& src)
		{
			target.robustBufferAccess = src.robustBufferAccess == VK_TRUE;
			target.fullDrawIndexUint32 = src.fullDrawIndexUint32 == VK_TRUE;
			target.imageCubeArray = src.imageCubeArray == VK_TRUE;
			target.independentBlend = src.independentBlend == VK_TRUE;
			target.geometryShader = src.geometryShader == VK_TRUE;
			target.tessellationShader = src.tessellationShader == VK_TRUE;
			target.sampleRateShading = src.sampleRateShading == VK_TRUE;
			target.dualSrcBlend = src.dualSrcBlend == VK_TRUE;
			target.logicOp = src.logicOp == VK_TRUE;
			target.multiDrawIndirect = src.multiDrawIndirect == VK_TRUE;
			target.drawIndirectFirstInstance = src.drawIndirectFirstInstance == VK_TRUE;
			target.depthClamp = src.depthClamp == VK_TRUE;
			target.depthBiasClamp = src.depthBiasClamp == VK_TRUE;
			target.fillModeNonSolid = src.fillModeNonSolid == VK_TRUE;
			target.depthBounds = src.depthBounds == VK_TRUE;
			target.wideLines = src.wideLines == VK_TRUE;
			target.largePoints = src.largePoints == VK_TRUE;
			target.alphaToOne = src.alphaToOne == VK_TRUE;
			target.multiViewport = src.multiViewport == VK_TRUE;
			target.samplerAnisotropy = src.samplerAnisotropy == VK_TRUE;
			target.textureCompressionETC2 = src.textureCompressionETC2 == VK_TRUE;
			target.textureCompressionASTC_LDR = src.textureCompressionASTC_LDR == VK_TRUE;
			target.textureCompressionBC = src.textureCompressionBC == VK_TRUE;
			target.occlusionQueryPrecise = src.occlusionQueryPrecise == VK_TRUE;
			target.pipelineStatisticsQuery = src.pipelineStatisticsQuery == VK_TRUE;
			target.vertexPipelineStoresAndAtomics = src.vertexPipelineStoresAndAtomics == VK_TRUE;
			target.fragmentStoresAndAtomics = src.fragmentStoresAndAtomics == VK_TRUE;
			target.shaderTessellationAndGeometryPointSize = src.shaderTessellationAndGeometryPointSize == VK_TRUE;
			target.shaderImageGatherExtended = src.shaderImageGatherExtended == VK_TRUE;
			target.shaderStorageImageExtendedFormats = src.shaderStorageImageExtendedFormats == VK_TRUE;
			target.shaderStorageImageMultisample = src.shaderStorageImageMultisample == VK_TRUE;
			target.shaderStorageImageReadWithoutFormat = src.shaderStorageImageReadWithoutFormat == VK_TRUE;
			target.shaderStorageImageWriteWithoutFormat = src.shaderStorageImageWriteWithoutFormat == VK_TRUE;
			target.shaderUniformBufferArrayDynamicIndexing = src.shaderUniformBufferArrayDynamicIndexing == VK_TRUE;
			target.shaderSampledImageArrayDynamicIndexing = src.shaderSampledImageArrayDynamicIndexing == VK_TRUE;
			target.shaderStorageBufferArrayDynamicIndexing = src.shaderStorageBufferArrayDynamicIndexing == VK_TRUE;
			target.shaderStorageImageArrayDynamicIndexing = src.shaderStorageImageArrayDynamicIndexing == VK_TRUE;
			target.shaderClipDistance = src.shaderClipDistance == VK_TRUE;
			target.shaderCullDistance = src.shaderCullDistance == VK_TRUE;
			target.shaderFloat64 = src.shaderFloat64 == VK_TRUE;
			target.shaderInt64 = src.shaderInt64 == VK_TRUE;
			target.shaderInt16 = src.shaderInt16 == VK_TRUE;
			target.shaderResourceResidency = src.shaderResourceResidency == VK_TRUE;
			target.shaderResourceMinLod = src.shaderResourceMinLod == VK_TRUE;
			target.sparseBinding = src.sparseBinding == VK_TRUE;
			target.sparseResidencyBuffer = src.sparseResidencyBuffer == VK_TRUE;
			target.sparseResidencyImage2D = src.sparseResidencyImage2D == VK_TRUE;
			target.sparseResidencyImage3D = src.sparseResidencyImage3D == VK_TRUE;
			target.sparseResidency2Samples = src.sparseResidency2Samples == VK_TRUE;
			target.sparseResidency4Samples = src.sparseResidency4Samples == VK_TRUE;
			target.sparseResidency8Samples = src.sparseResidency8Samples == VK_TRUE;
			target.sparseResidency16Samples = src.sparseResidency16Samples == VK_TRUE;
			target.sparseResidencyAliased = src.sparseResidencyAliased == VK_TRUE;
			target.variableMultisampleRate = src.variableMultisampleRate == VK_TRUE;
			target.inheritedQueries = src.inheritedQueries == VK_TRUE;
		}
	} // namespace

	const physical_device_features& physical_device::get_features(bool forceRefresh)
	{
		auto* impl = get_native_ptr(*this);

		if (forceRefresh || !impl->featuresLoaded)
		{
			VkPhysicalDeviceFeatures features;
			impl->vkGetPhysicalDeviceFeatures(impl->physicalDevice, &features);
			map_vk_physical_device_features(impl->features, features);

			impl->featuresLoaded = true;
		}

		return impl->features;
	}

	namespace
	{
		physical_device_type map_vk_physical_device_type(VkPhysicalDeviceType type)
		{
			switch (type)
			{
				case VK_PHYSICAL_DEVICE_TYPE_OTHER: return physical_device_type::Other;
				case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return physical_device_type::Integrated;
				case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return physical_device_type::Discrete;
				case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return physical_device_type::Virtual;
				case VK_PHYSICAL_DEVICE_TYPE_CPU: return physical_device_type::CPU;
				case VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM: return physical_device_type::Other;
			}

			return physical_device_type::Other;
		}

		void map_vk_physical_device_limits(physical_device_limits& target, const VkPhysicalDeviceLimits& src)
		{
			target.maxImageDimension1D = src.maxImageDimension1D;
			target.maxImageDimension2D = src.maxImageDimension2D;
			target.maxImageDimension3D = src.maxImageDimension3D;
			target.maxImageDimensionCube = src.maxImageDimensionCube;
			target.maxImageArrayLayers = src.maxImageArrayLayers;
			target.maxTexelBufferElements = src.maxTexelBufferElements;
			target.maxUniformBufferRange = src.maxUniformBufferRange;
			target.maxStorageBufferRange = src.maxStorageBufferRange;
			target.maxPushConstantsSize = src.maxPushConstantsSize;
			target.maxMemoryAllocationCount = src.maxMemoryAllocationCount;
			target.maxSamplerAllocationCount = src.maxSamplerAllocationCount;
			target.bufferImageGranularity = src.bufferImageGranularity;
			target.sparseAddressSpaceSize = src.sparseAddressSpaceSize;
			target.maxBoundDescriptorSets = src.maxBoundDescriptorSets;
			target.maxPerStageDescriptorSamplers = src.maxPerStageDescriptorSamplers;
			target.maxPerStageDescriptorUniformBuffers = src.maxPerStageDescriptorUniformBuffers;
			target.maxPerStageDescriptorStorageBuffers = src.maxPerStageDescriptorStorageBuffers;
			target.maxPerStageDescriptorSampledImages = src.maxPerStageDescriptorSampledImages;
			target.maxPerStageDescriptorStorageImages = src.maxPerStageDescriptorStorageImages;
			target.maxPerStageDescriptorInputAttachments = src.maxPerStageDescriptorInputAttachments;
			target.maxPerStageResources = src.maxPerStageResources;
			target.maxDescriptorSetSamplers = src.maxDescriptorSetSamplers;
			target.maxDescriptorSetUniformBuffers = src.maxDescriptorSetUniformBuffers;
			target.maxDescriptorSetUniformBuffersDynamic = src.maxDescriptorSetUniformBuffersDynamic;
			target.maxDescriptorSetStorageBuffers = src.maxDescriptorSetStorageBuffers;
			target.maxDescriptorSetStorageBuffersDynamic = src.maxDescriptorSetStorageBuffersDynamic;
			target.maxDescriptorSetSampledImages = src.maxDescriptorSetSampledImages;
			target.maxDescriptorSetStorageImages = src.maxDescriptorSetStorageImages;
			target.maxDescriptorSetInputAttachments = src.maxDescriptorSetInputAttachments;
			target.maxVertexInputAttributes = src.maxVertexInputAttributes;
			target.maxVertexInputBindings = src.maxVertexInputBindings;
			target.maxVertexInputAttributeOffset = src.maxVertexInputAttributeOffset;
			target.maxVertexInputBindingStride = src.maxVertexInputBindingStride;
			target.maxVertexOutputComponents = src.maxVertexOutputComponents;
			target.maxTessellationGenerationLevel = src.maxTessellationGenerationLevel;
			target.maxTessellationPatchSize = src.maxTessellationPatchSize;
			target.maxTessellationControlPerVertexInputComponents = src.maxTessellationControlPerVertexInputComponents;
			target.maxTessellationControlPerVertexOutputComponents =
				src.maxTessellationControlPerVertexOutputComponents;
			target.maxTessellationControlPerPatchOutputComponents = src.maxTessellationControlPerPatchOutputComponents;
			target.maxTessellationControlTotalOutputComponents = src.maxTessellationControlTotalOutputComponents;
			target.maxTessellationEvaluationInputComponents = src.maxTessellationEvaluationInputComponents;
			target.maxTessellationEvaluationOutputComponents = src.maxTessellationEvaluationOutputComponents;
			target.maxGeometryShaderInvocations = src.maxGeometryShaderInvocations;
			target.maxGeometryInputComponents = src.maxGeometryInputComponents;
			target.maxGeometryOutputComponents = src.maxGeometryOutputComponents;
			target.maxGeometryOutputVertices = src.maxGeometryOutputVertices;
			target.maxGeometryTotalOutputComponents = src.maxGeometryTotalOutputComponents;
			target.maxFragmentInputComponents = src.maxFragmentInputComponents;
			target.maxFragmentOutputAttachments = src.maxFragmentOutputAttachments;
			target.maxFragmentDualSrcAttachments = src.maxFragmentDualSrcAttachments;
			target.maxFragmentCombinedOutputResources = src.maxFragmentCombinedOutputResources;
			target.maxComputeSharedMemorySize = src.maxComputeSharedMemorySize;
			target.maxComputeWorkGroupCount[0] = src.maxComputeWorkGroupCount[0];
			target.maxComputeWorkGroupCount[1] = src.maxComputeWorkGroupCount[1];
			target.maxComputeWorkGroupCount[2] = src.maxComputeWorkGroupCount[2];
			target.maxComputeWorkGroupInvocations = src.maxComputeWorkGroupInvocations;
			target.maxComputeWorkGroupSize[0] = src.maxComputeWorkGroupSize[0];
			target.maxComputeWorkGroupSize[1] = src.maxComputeWorkGroupSize[1];
			target.maxComputeWorkGroupSize[2] = src.maxComputeWorkGroupSize[2];
			target.subPixelPrecisionBits = src.subPixelPrecisionBits;
			target.subTexelPrecisionBits = src.subTexelPrecisionBits;
			target.mipmapPrecisionBits = src.mipmapPrecisionBits;
			target.maxDrawIndexedIndexValue = src.maxDrawIndexedIndexValue;
			target.maxDrawIndirectCount = src.maxDrawIndirectCount;
			target.maxSamplerLodBias = src.maxSamplerLodBias;
			target.maxSamplerAnisotropy = src.maxSamplerAnisotropy;
			target.maxViewports = src.maxViewports;
			target.maxViewportDimensions[0] = src.maxViewportDimensions[0];
			target.maxViewportDimensions[1] = src.maxViewportDimensions[1];
			target.viewportBoundsRange[0] = src.viewportBoundsRange[0];
			target.viewportBoundsRange[1] = src.viewportBoundsRange[1];
			target.viewportSubPixelBits = src.viewportSubPixelBits;
			target.minMemoryMapAlignment = src.minMemoryMapAlignment;
			target.minTexelBufferOffsetAlignment = src.minTexelBufferOffsetAlignment;
			target.minUniformBufferOffsetAlignment = src.minUniformBufferOffsetAlignment;
			target.minStorageBufferOffsetAlignment = src.minStorageBufferOffsetAlignment;
			target.minTexelOffset = src.minTexelOffset;
			target.maxTexelOffset = src.maxTexelOffset;
			target.minTexelGatherOffset = src.minTexelGatherOffset;
			target.maxTexelGatherOffset = src.maxTexelGatherOffset;
			target.minInterpolationOffset = src.minInterpolationOffset;
			target.maxInterpolationOffset = src.maxInterpolationOffset;
			target.subPixelInterpolationOffsetBits = src.subPixelInterpolationOffsetBits;
			target.maxFramebufferWidth = src.maxFramebufferWidth;
			target.maxFramebufferHeight = src.maxFramebufferHeight;
			target.maxFramebufferLayers = src.maxFramebufferLayers;
			target.framebufferColorSampleCounts = static_cast<sample_count_flags>(src.framebufferColorSampleCounts);
			target.framebufferDepthSampleCounts = static_cast<sample_count_flags>(src.framebufferDepthSampleCounts);
			target.framebufferStencilSampleCounts = static_cast<sample_count_flags>(src.framebufferStencilSampleCounts);
			target.framebufferNoAttachmentsSampleCounts =
				static_cast<sample_count_flags>(src.framebufferNoAttachmentsSampleCounts);
			target.maxColorAttachments = src.maxColorAttachments;
			target.sampledImageColorSampleCounts = static_cast<sample_count_flags>(src.sampledImageColorSampleCounts);
			target.sampledImageIntegerSampleCounts =
				static_cast<sample_count_flags>(src.sampledImageIntegerSampleCounts);
			target.sampledImageDepthSampleCounts = static_cast<sample_count_flags>(src.sampledImageDepthSampleCounts);
			target.sampledImageStencilSampleCounts =
				static_cast<sample_count_flags>(src.sampledImageStencilSampleCounts);
			target.storageImageSampleCounts = static_cast<sample_count_flags>(src.storageImageSampleCounts);
			target.maxSampleMaskWords = src.maxSampleMaskWords;
			target.timestampComputeAndGraphics = src.timestampComputeAndGraphics == VK_TRUE;
			target.timestampPeriod = src.timestampPeriod;
			target.maxClipDistances = src.maxClipDistances;
			target.maxCullDistances = src.maxCullDistances;
			target.maxCombinedClipAndCullDistances = src.maxCombinedClipAndCullDistances;
			target.discreteQueuePriorities = src.discreteQueuePriorities;
			target.pointSizeRange[0] = src.pointSizeRange[0];
			target.pointSizeRange[1] = src.pointSizeRange[1];
			target.lineWidthRange[0] = src.lineWidthRange[0];
			target.lineWidthRange[1] = src.lineWidthRange[1];
			target.pointSizeGranularity = src.pointSizeGranularity;
			target.lineWidthGranularity = src.lineWidthGranularity;
			target.strictLines = src.strictLines == VK_TRUE;
			target.standardSampleLocations = src.standardSampleLocations == VK_TRUE;
			target.optimalBufferCopyOffsetAlignment = src.optimalBufferCopyOffsetAlignment;
			target.optimalBufferCopyRowPitchAlignment = src.optimalBufferCopyRowPitchAlignment;
			target.nonCoherentAtomSize = src.nonCoherentAtomSize;
		}

		void map_vk_physical_device_sparse_properties(
			physical_device_sparse_properties& target, const VkPhysicalDeviceSparseProperties& src
		)
		{
			target.residencyStandard2DBlockShape = src.residencyStandard2DBlockShape == VK_TRUE;
			target.residencyStandard2DMultisampleBlockShape = src.residencyStandard2DMultisampleBlockShape == VK_TRUE;
			target.residencyStandard3DBlockShape = src.residencyStandard3DBlockShape == VK_TRUE;
			target.residencyAlignedMipSize = src.residencyAlignedMipSize == VK_TRUE;
			target.residencyNonResidentStrict = src.residencyNonResidentStrict == VK_TRUE;
		}


	} // namespace

	physical_device::operator bool() const noexcept
	{
		auto ptr = get_native_ptr(*this);
		return ptr != nullptr && ptr->physicalDevice != VK_NULL_HANDLE;
	}

	const physical_device_properties& physical_device::get_properties(bool forceRefresh)
	{
		auto* impl = get_native_ptr(*this);

		if (forceRefresh || !impl->propertiesLoaded)
		{
			VkPhysicalDeviceProperties props;
			impl->vkGetPhysicalDeviceProperties(impl->physicalDevice, &props);

			impl->properties.apiVersion = decomposeVkVersion(props.apiVersion);
			impl->properties.driverVersion = decomposeVkVersion(props.driverVersion);
			impl->properties.vendorID = props.vendorID;
			impl->properties.deviceID = props.deviceID;
			impl->properties.deviceType = map_vk_physical_device_type(props.deviceType);
			impl->properties.deviceName = props.deviceName;
			map_vk_physical_device_limits(impl->properties.limits, props.limits);
			map_vk_physical_device_sparse_properties(impl->properties.sparseProperties, props.sparseProperties);

			impl->propertiesLoaded = true;
		}

		return impl->properties;
	}

	bool physical_device::initialize(std::span<const char*> extensions)
	{
		return get_native_ptr(*this)->load_functions(extensions);
	}

	bool physical_device::in_use() const
	{
		return get_native_ptr(*this)->renderDevice;
	}

	render_device
	physical_device::create_render_device([[maybe_unused]] std::span<const queue_description> queueDesciptions)
	{
		auto* impl = get_native_ptr(*this);

		struct queue_construction_info
		{
			rsl::size_type familyIndex = -1ull;
			rsl::size_type score = 0;
			queue_priority priority;
		};

		std::vector<queue_construction_info> queueConstructionInfos;
		queueConstructionInfos.resize(queueDesciptions.size());

		for (rsl::size_type i = 0; i < queueDesciptions.size(); i++)
		{
			queueConstructionInfos[i].priority = queueDesciptions[i].priority;
			queueConstructionInfos[i].familyIndex = queueDesciptions[i].queueFamilyIndexOverride;
		}

		auto queueFamilies = get_available_queue_families();

		rsl::size_type familyIndex = 0;
		for (auto& queueFamily : queueFamilies)
		{
			rsl::size_type queueIndex = 0;
			for (auto& queueDesciption : queueDesciptions)
			{
				auto& queueInfo = queueConstructionInfos[queueIndex];

				if (queueDesciption.queueFamilyIndexOverride != -1ull ||
					!rsl::enum_flags::has_all_flags(queueFamily.features, queueDesciption.requiredFeatures) ||
					queueFamily.queueCount == 0)
				{
					queueIndex++;
					continue;
				}

				rsl::size_type score = 1;

				score += queueFamily.queueCount * queueDesciption.queueCountImportance;
				score += queueFamily.timestampValidBits * queueDesciption.timestampImportance;

				if (queueDesciption.imageTransferGranularityImportance != 0ull)
				{
					rsl::size_type maxScore = 128ull * queueDesciption.imageTransferGranularityImportance;

					score += maxScore - rsl::math::min(
											maxScore, ((queueFamily.minImageTransferGranularity.x +
														queueFamily.minImageTransferGranularity.y +
														queueFamily.minImageTransferGranularity.z) /
													   3u) *
														  queueDesciption.imageTransferGranularityImportance
										);
				}

				if (score > queueInfo.score)
				{
					queueInfo.familyIndex = familyIndex;
					queueInfo.score = score;
				}

				queueIndex++;
			}
			familyIndex++;
		}

		std::cout << "selected queues:\n";

		rsl::size_type queueIndex = 0;
		for (auto& [familyIndex, score, priority] : queueConstructionInfos)
		{
			std::cout << "\t" << queueIndex << ":\n";

			if (familyIndex >= queueFamilies.size())
			{
				std::cout << "\t\tNOT FOUND\n";
				continue;
			}

			auto& queueFamily = queueFamilies[familyIndex];

			std::cout << "\t\tscore: " << score << '\n';
			std::cout << "\t\tpriority: " << to_string(priority) << '\n';
			std::cout << "\t\tallowed operations:\n";

#define PRINT_FEATURE(name)                                                                                            \
	if (rsl::enum_flags::has_flag(queueFamily.features, vk::queue_feature_flags::name))                                \
	{                                                                                                                  \
		std::cout << "\t\t\t\t" #name "\n";                                                                            \
	}

			PRINT_FEATURE(Graphics);
			PRINT_FEATURE(Compute);
			PRINT_FEATURE(Transfer);
			PRINT_FEATURE(SparseBinding);
			PRINT_FEATURE(Protected);
			PRINT_FEATURE(VideoDecode);
			PRINT_FEATURE(VideoEncode);
			PRINT_FEATURE(OpticalFlowNV);

#undef PRINT_FEATURE

			std::cout << "\t\tallowed amount: " << queueFamily.queueCount << '\n';
			std::cout << "\t\ttimestamp valid bits: " << queueFamily.timestampValidBits << '\n';
			std::cout << "\t\tminimum image transfer resolution:\n";
			std::cout << "\t\t\twidth: " << queueFamily.minImageTransferGranularity.x << '\n';
			std::cout << "\t\t\theight: " << queueFamily.minImageTransferGranularity.y << '\n';
			std::cout << "\t\t\tdepth: " << queueFamily.minImageTransferGranularity.z << '\n';

			queueIndex++;
		}

		return impl->renderDevice;
	}

	void physical_device::release_render_device()
	{
		auto* impl = get_native_ptr(*this);

		if (auto ptr = get_native_ptr(impl->renderDevice); ptr != nullptr)
		{
			delete ptr;

			impl->renderDevice.m_nativeRenderDevice = invalid_native_render_device;
		}
	}

	bool native_physical_device_vk::load_functions([[maybe_unused]] std::span<const char*> extensions)
	{
		return false;
	}

	render_device::operator bool() const noexcept
	{
		auto ptr = get_native_ptr(*this);
		return ptr != nullptr && ptr->device != VK_NULL_HANDLE;
	}

} // namespace vk
