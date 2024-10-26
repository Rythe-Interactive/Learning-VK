#include "vulkan.hpp"

#include <bit>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#if RYTHE_PLATFORM_WINDOWS
	#define VK_USE_PLATFORM_WIN32_KHR
	#include <vulkan/vulkan_win32.h>
	#define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif RYTHE_PLATFORM_LINUX
	#ifdef RYTHE_SURFACE_XCB
		#define VK_USE_PLATFORM_XCB_KHR
		#include <vulkan/vulkan_xcb.h>
		#define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME
	#endif
	#ifdef RYTHE_SURFACE_XLIB
		#define VK_USE_PLATFORM_XLIB_KHR
		#include <vulkan/vulkan_xlib.h>
		#define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_XLIB_SURFACE_EXTENSION_NAME
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

#if RYTHE_PLATFORM_WINDOWS
	native_window_handle create_window_handle_win32(HWND hwnd)
	{
		return std::bit_cast<native_window_handle>(hwnd);
	}

	HWND get_native_window(native_window_handle handle)
	{
		return std::bit_cast<HWND>(handle);
	}
#endif

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
	name = std::bit_cast<PFN_##name>(vkGetInstanceProcAddr(nullptr, #name));                                           \
	if (!name)                                                                                                         \
	{                                                                                                                  \
		std::cout << "Could not load global-level Vulkan function \"" #name "\"\n";                                    \
		return false;                                                                                                  \
	}

#include "impl/list_of_vulkan_functions.inl"

		libraryIsInitialized = true;

		return true;
	}

	void shut_down()
	{
		libraryIsInitialized = false;

#define EXPORTED_VULKAN_FUNCTION(name) name = nullptr;
#define GLOBAL_LEVEL_VULKAN_FUNCTION(name) name = nullptr;
#include "impl/list_of_vulkan_functions.inl"

		vulkanLibrary.release();
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

	template <typename T>
	struct native_handle_traits
	{
	};

	struct native_instance_vk
	{
		bool load_functions(std::span<rsl::cstring> extensions);

#define INSTANCE_LEVEL_VULKAN_FUNCTION(name) [[maybe_unused]] PFN_##name name = nullptr;
#define INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(name, extension) [[maybe_unused]] PFN_##name name = nullptr;
#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION(name) [[maybe_unused]] PFN_##name name = nullptr;
#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION_FROM_EXTENSION(name, extension)                                 \
	[[maybe_unused]] PFN_##name name = nullptr;
#define INSTANCE_LEVEL_DEVICE_VULKAN_FUNCTION(name) [[maybe_unused]] PFN_##name name = nullptr;
#include "impl/list_of_vulkan_functions.inl"

		std::vector<physical_device> physicalDevices;
		application_info applicationInfo;
		semver::version apiVersion;

		VkInstance instance = VK_NULL_HANDLE;
	};

	static void set_native_handle(instance& target, native_instance handle)
	{
		target.m_nativeInstance = handle;
	}

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

	struct native_physical_device_vk
	{
#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION(name) [[maybe_unused]] PFN_##name name = nullptr;
#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION_FROM_EXTENSION(name, extension)                                 \
	[[maybe_unused]] PFN_##name name = nullptr;
#define INSTANCE_LEVEL_DEVICE_VULKAN_FUNCTION(name) [[maybe_unused]] PFN_##name name = nullptr;
#include "impl/list_of_vulkan_functions.inl"

		render_device renderDevice;

		bool featuresLoaded = false;
		physical_device_features features;
		bool propertiesLoaded = false;
		physical_device_properties properties;
		std::vector<extension_properties> availableExtensions;
		std::vector<queue_family_properties> availableQueueFamilies;

		instance instance;

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	};

	static void set_native_handle(physical_device& target, native_physical_device handle)
	{
		target.m_nativePhysicalDevice = handle;
	}

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

	struct native_render_device_vk
	{
		bool load_functions(std::span<rsl::cstring> extensions);

#define INSTANCE_LEVEL_DEVICE_VULKAN_FUNCTION(name) [[maybe_unused]] PFN_##name name = nullptr;
#define DEVICE_LEVEL_VULKAN_FUNCTION(name) [[maybe_unused]] PFN_##name name = nullptr;
#define DEVICE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(name, extension) [[maybe_unused]] PFN_##name name = nullptr;
#include "impl/list_of_vulkan_functions.inl"

		physical_device physicalDevice;

		std::vector<queue> queues;

		VkDevice device = VK_NULL_HANDLE;
	};

	static void set_native_handle(render_device& target, native_render_device handle)
	{
		target.m_nativeRenderDevice = handle;
	}

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

	struct native_queue_vk
	{
		render_device renderDevice;

		rsl::size_type queueIndex;
		rsl::size_type familyIndex;
		queue_priority priority;
		VkQueue queue = VK_NULL_HANDLE;
	};

	static void set_native_handle(queue& target, native_queue handle)
	{
		target.m_nativeQueue = handle;
	}

	template <>
	struct native_handle_traits<queue>
	{
		using native_type = native_queue_vk;
		using handle_type = native_queue;
		constexpr static handle_type invalid_handle = invalid_native_queue;
	};

	template <>
	struct native_handle_traits<native_queue_vk>
	{
		using api_type = queue;
		using handle_type = native_queue;
		constexpr static handle_type invalid_handle = invalid_native_queue;
	};

	namespace
	{
		template <typename T>
		rythe_always_inline typename native_handle_traits<T>::native_type* get_native_ptr(const T& inst)
		{
			return std::bit_cast<typename native_handle_traits<T>::native_type*>(inst.get_native_handle());
		}

		template <typename T>
		rythe_always_inline auto create_native_handle(T* inst)
		{
			return std::bit_cast<typename native_handle_traits<T>::handle_type>(inst);
		}
	} // namespace

	instance create_instance(
		const application_info& _applicationInfo, const semver::version& apiVersion, std::span<rsl::cstring> extensions
	)
	{
		std::vector<rsl::cstring> enabledExtensions;
		bool surfaceExtensionActive = false;
		bool platformSurfaceExtensionActive = false;
		for (auto& extensionName : extensions)
		{
			auto nameView = std::string_view(extensionName);
			if (!is_instance_extension_available(nameView))
			{
				std::cout << "Extension \"" << extensionName << "\" is not available.\n";
			}
			else
			{
				if (nameView == VK_KHR_SURFACE_EXTENSION_NAME)
				{
					surfaceExtensionActive = true;
				}
				else if (nameView == VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME)
				{
					platformSurfaceExtensionActive = true;
				}

				enabledExtensions.push_back(extensionName);
			}
		}

		if (_applicationInfo.windowHandle != invalid_native_window_handle)
		{
			if (!surfaceExtensionActive)
			{
				enabledExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
			}

			if (!platformSurfaceExtensionActive)
			{
				enabledExtensions.push_back(VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME);
			}
		}
		else
		{
			if (surfaceExtensionActive)
			{
				std::cout << "Surface extension is activated, but no window handle was provided.\n";
				return {};
			}

			if (platformSurfaceExtensionActive)
			{
				std::cout << "Platform specific surface extension is activated, but no window handle was provided.\n";
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
			.enabledExtensionCount = static_cast<rsl::uint32>(enabledExtensions.size()),
			.ppEnabledExtensionNames = enabledExtensions.data(),
		};

		VkInstance vkInstance = VK_NULL_HANDLE;
		VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &vkInstance);

		if (result != VK_SUCCESS || vkInstance == VK_NULL_HANDLE)
		{
			std::cout << "Failed to create Vulkan Instance\n";
			return {};
		}

		native_instance_vk* nativeInstance = new native_instance_vk();
		nativeInstance->instance = vkInstance;

		if (!nativeInstance->load_functions(enabledExtensions))
		{
			if (nativeInstance->vkDestroyInstance)
			{
				nativeInstance->vkDestroyInstance(nativeInstance->instance, nullptr);
			}

			delete nativeInstance;
			return {};
		}

		nativeInstance->applicationInfo = _applicationInfo;
		nativeInstance->apiVersion = apiVersion;

		vk::instance instance;
		set_native_handle(instance, create_native_handle(nativeInstance));

		return instance;
	}

	std::string_view to_string(physical_device_type type)
	{
		switch (type)
		{
			case physical_device_type::other: return "other";
			case physical_device_type::integratedGPU: return "integrated GPU";
			case physical_device_type::discreteGPU: return "discrete GPU";
			case physical_device_type::virtualGPU: return "virtual GPU";
			case physical_device_type::CPU: return "CPU";
		}

		return "unknown";
	}

	std::string_view to_string(queue_priority priority)
	{
		switch (priority)
		{
			case queue_priority::normal: return "normal";
			case queue_priority::high: return "high";
		}

		return "unknown";
	}

	instance::operator bool() const noexcept
	{
		auto ptr = get_native_ptr(*this);
		return ptr != nullptr && ptr->instance != VK_NULL_HANDLE;
	}

	void instance::release()
	{
		auto impl = get_native_ptr(*this);
		if (!impl)
		{
			return;
		}

		release_physical_devices();

		impl->vkDestroyInstance(impl->instance, nullptr);

		m_nativeInstance = invalid_native_instance;
		delete impl;
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

			release_physical_devices();
			impl->physicalDevices.reserve(deviceCount);
			for (auto& pd : physicalDevicesBuffer)
			{
				auto& physicalDevice = impl->physicalDevices.emplace_back();
				native_physical_device_vk* nativePhysicalDevice = new native_physical_device_vk();
				set_native_handle(physicalDevice, create_native_handle(nativePhysicalDevice));

				nativePhysicalDevice->physicalDevice = pd;
				nativePhysicalDevice->instance = *this;

#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION(name) nativePhysicalDevice->name = impl->name;
#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION_FROM_EXTENSION(name, extension)                                 \
	nativePhysicalDevice->name = impl->name;
#define INSTANCE_LEVEL_DEVICE_VULKAN_FUNCTION(name) nativePhysicalDevice->name = impl->name;
#include "impl/list_of_vulkan_functions.inl"
			}
		}

		return impl->physicalDevices;
	}

	void instance::release_physical_devices()
	{
		auto* impl = get_native_ptr(*this);

		for (auto& device : impl->physicalDevices) { device.release(); }

		impl->physicalDevices.clear();
	}

	const application_info& instance::get_application_info() const noexcept
	{
		return get_native_ptr(*this)->applicationInfo;
	}

	const semver::version& instance::get_api_version() const noexcept
	{
		return get_native_ptr(*this)->apiVersion;
	}

	namespace
	{
		physical_device copy_physical_device(physical_device src)
		{
			physical_device copy;
			set_native_handle(copy, create_native_handle(new native_physical_device_vk(*get_native_ptr(src))));

			return copy;
		}

		render_device create_render_device_no_extension_check(
			physical_device& physicalDevice, std::span<const queue_description> queueDesciptions,
			std::span<rsl::cstring> extensions
		)
		{
			auto* impl = get_native_ptr(physicalDevice);

			std::vector<queue_family_selection> queueFamilySelections;
			queueFamilySelections.resize(queueDesciptions.size());

			if (!physicalDevice.get_queue_family_selection(queueFamilySelections, queueDesciptions))
			{
				return {};
			}

			auto queueFamilies = physicalDevice.get_available_queue_families();

			struct queue_mapping
			{
				std::vector<rsl::size_type> inputOrderIndex;
				std::vector<rsl::float32> priorities;
			};

			std::vector<queue_mapping> queueMapping;
			queueMapping.resize(queueFamilies.size());

			for (rsl::size_type i = 0; i < queueDesciptions.size(); i++)
			{
				auto familyIndex = queueFamilySelections[i].familyIndex;

				queueMapping[familyIndex].inputOrderIndex.push_back(i);
				queueMapping[familyIndex].priorities.push_back(
					queueDesciptions[i].priority == queue_priority::normal ? 0.5f : 1.f
				);
			}

			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
			queueCreateInfos.reserve(queueDesciptions.size());
			for (rsl::size_type i = 0; i < queueMapping.size(); i++)
			{
				auto& priorities = queueMapping[i].priorities;

				if (!priorities.empty())
				{
					queueCreateInfos.push_back(VkDeviceQueueCreateInfo{
						.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
						.pNext = nullptr,
						.flags = 0,
						.queueFamilyIndex = static_cast<rsl::uint32>(i),
						.queueCount = static_cast<rsl::uint32>(priorities.size()),
						.pQueuePriorities = priorities.data(),
					});
				}
			}

			VkPhysicalDeviceFeatures features;
			impl->vkGetPhysicalDeviceFeatures(impl->physicalDevice, &features);

			VkDeviceCreateInfo deviceCreateInfo{
				.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.queueCreateInfoCount = static_cast<rsl::uint32>(queueCreateInfos.size()),
				.pQueueCreateInfos = queueCreateInfos.data(),
				.enabledLayerCount = 0,
				.ppEnabledLayerNames = nullptr,
				.enabledExtensionCount = static_cast<rsl::uint32>(extensions.size()),
				.ppEnabledExtensionNames = extensions.data(),
				.pEnabledFeatures = &features,
			};

			VkDevice device = VK_NULL_HANDLE;
			VkResult result = impl->vkCreateDevice(impl->physicalDevice, &deviceCreateInfo, nullptr, &device);

			if (result != VK_SUCCESS || device == VK_NULL_HANDLE)
			{
				return {};
			}

			auto* renderDevicePtr = new native_render_device_vk();
			renderDevicePtr->device = device;

#define INSTANCE_LEVEL_DEVICE_VULKAN_FUNCTION(name) renderDevicePtr->name = impl->name;
#include "impl/list_of_vulkan_functions.inl"

			if (!renderDevicePtr->load_functions(extensions))
			{
				if (renderDevicePtr->vkDestroyDevice)
				{
					renderDevicePtr->vkDestroyDevice(renderDevicePtr->device, nullptr);
				}

				delete renderDevicePtr;
				return {};
			}

			renderDevicePtr->queues.resize(queueDesciptions.size());
			for (rsl::size_type i = 0; i < queueCreateInfos.size(); i++)
			{
				auto& info = queueCreateInfos[i];
				auto& mapping = queueMapping[info.queueFamilyIndex];

				for (rsl::size_type queueIndex = 0; queueIndex < info.queueCount; queueIndex++)
				{
					VkQueue vkQueue = VK_NULL_HANDLE;
					renderDevicePtr->vkGetDeviceQueue(
						renderDevicePtr->device, info.queueFamilyIndex, queueIndex, &vkQueue
					);

					if (vkQueue == VK_NULL_HANDLE)
					{
						// not sure yet
						continue;
					}

					auto inputIndex = mapping.inputOrderIndex[queueIndex];
					auto& queue = renderDevicePtr->queues[inputIndex];

					native_queue_vk* nativeQueue = new native_queue_vk();
					nativeQueue->queue = vkQueue;
					nativeQueue->renderDevice = impl->renderDevice;
					nativeQueue->queueIndex = inputIndex;
					nativeQueue->familyIndex = info.queueFamilyIndex;
					nativeQueue->priority = queueDesciptions[inputIndex].priority;

					set_native_handle(queue, create_native_handle(nativeQueue));
				}
			}

			renderDevicePtr->physicalDevice = copy_physical_device(physicalDevice);

			set_native_handle(impl->renderDevice, create_native_handle(renderDevicePtr));
			return impl->renderDevice;
		}
	} // namespace

	render_device instance::auto_select_and_create_device(
		const physical_device_description& physicalDeviceDescription,
		std::span<const queue_description> queueDesciptions, std::span<rsl::cstring> extensions
	)
	{
		using namespace semver::literals;
		const bool supportsProtectedMemory = get_api_version() >= "1.1.0"_version;

		for (rsl::size_type i = 0; i < queueDesciptions.size(); i++)
		{
			if (!supportsProtectedMemory &&
				rsl::enum_flags::has_flag(queueDesciptions[i].requiredFeatures, queue_feature_flags::protectedMemory))
			{
				std::cout << "Protected memory operations are not supported for Vulkan versions prior to 1.1.0\n";
				return {};
			}
		}

		auto physicalDevices = create_physical_devices();

		rsl::size_type selectedDevice = rsl::npos;
		rsl::size_type currentScore = 0;

		rsl::size_type deviceIndex = 0;
		for (auto& device : physicalDevices)
		{
			auto& props = device.get_properties();

			if (props.apiVersion < physicalDeviceDescription.apiVersion)
			{
				continue;
			}

			if (props.limits.maxPerStageDescriptorSampledImages <
				physicalDeviceDescription.requiredPerStageSampledImages)
			{
				continue;
			}

			auto& features = device.get_features();

#define CHECK_FEATURE(feature)                                                                                         \
	if (physicalDeviceDescription.requiredFeatures.feature && !features.feature)                                       \
	{                                                                                                                  \
		continue;                                                                                                      \
	}

			CHECK_FEATURE(robustBufferAccess);
			CHECK_FEATURE(fullDrawIndexUint32);
			CHECK_FEATURE(imageCubeArray);
			CHECK_FEATURE(independentBlend);
			CHECK_FEATURE(geometryShader);
			CHECK_FEATURE(tessellationShader);
			CHECK_FEATURE(sampleRateShading);
			CHECK_FEATURE(dualSrcBlend);
			CHECK_FEATURE(logicOp);
			CHECK_FEATURE(multiDrawIndirect);
			CHECK_FEATURE(drawIndirectFirstInstance);
			CHECK_FEATURE(depthClamp);
			CHECK_FEATURE(depthBiasClamp);
			CHECK_FEATURE(fillModeNonSolid);
			CHECK_FEATURE(depthBounds);
			CHECK_FEATURE(wideLines);
			CHECK_FEATURE(largePoints);
			CHECK_FEATURE(alphaToOne);
			CHECK_FEATURE(multiViewport);
			CHECK_FEATURE(samplerAnisotropy);
			CHECK_FEATURE(textureCompressionETC2);
			CHECK_FEATURE(textureCompressionASTC_LDR);
			CHECK_FEATURE(textureCompressionBC);
			CHECK_FEATURE(occlusionQueryPrecise);
			CHECK_FEATURE(pipelineStatisticsQuery);
			CHECK_FEATURE(vertexPipelineStoresAndAtomics);
			CHECK_FEATURE(fragmentStoresAndAtomics);
			CHECK_FEATURE(shaderTessellationAndGeometryPointSize);
			CHECK_FEATURE(shaderImageGatherExtended);
			CHECK_FEATURE(shaderStorageImageExtendedFormats);
			CHECK_FEATURE(shaderStorageImageMultisample);
			CHECK_FEATURE(shaderStorageImageReadWithoutFormat);
			CHECK_FEATURE(shaderStorageImageWriteWithoutFormat);
			CHECK_FEATURE(shaderUniformBufferArrayDynamicIndexing);
			CHECK_FEATURE(shaderSampledImageArrayDynamicIndexing);
			CHECK_FEATURE(shaderStorageBufferArrayDynamicIndexing);
			CHECK_FEATURE(shaderStorageImageArrayDynamicIndexing);
			CHECK_FEATURE(shaderClipDistance);
			CHECK_FEATURE(shaderCullDistance);
			CHECK_FEATURE(shaderFloat64);
			CHECK_FEATURE(shaderInt64);
			CHECK_FEATURE(shaderInt16);
			CHECK_FEATURE(shaderResourceResidency);
			CHECK_FEATURE(shaderResourceMinLod);
			CHECK_FEATURE(sparseBinding);
			CHECK_FEATURE(sparseResidencyBuffer);
			CHECK_FEATURE(sparseResidencyImage2D);
			CHECK_FEATURE(sparseResidencyImage3D);
			CHECK_FEATURE(sparseResidency2Samples);
			CHECK_FEATURE(sparseResidency4Samples);
			CHECK_FEATURE(sparseResidency8Samples);
			CHECK_FEATURE(sparseResidency16Samples);
			CHECK_FEATURE(sparseResidencyAliased);
			CHECK_FEATURE(variableMultisampleRate);
			CHECK_FEATURE(inheritedQueries);

#undef CHECK_FEATURE

			for (auto& extensionName : extensions)
			{
				if (!device.is_extension_available(extensionName))
				{
					continue;
				}
			}

			std::vector<queue_family_selection> queueFamilySelections;
			queueFamilySelections.resize(queueDesciptions.size());
			if (!device.get_queue_family_selection(queueFamilySelections, queueDesciptions))
			{
				continue;
			}

			rsl::size_type deviceScore = 1;

			deviceScore +=
				physicalDeviceDescription.deviceTypeImportance[static_cast<rsl::size_type>(props.deviceType)];

			deviceScore += props.limits.maxImageDimension2D;

			if (deviceScore > currentScore)
			{
				selectedDevice = deviceIndex;
				currentScore = deviceScore;
			}

			deviceIndex++;
		}

		if (selectedDevice >= physicalDevices.size())
		{
			return render_device();
		}

		auto result =
			create_render_device_no_extension_check(physicalDevices[selectedDevice], queueDesciptions, extensions);

		release_physical_devices();

		return result;
	}

	bool native_instance_vk::load_functions([[maybe_unused]] std::span<rsl::cstring> extensions)
	{
#define INSTANCE_LEVEL_VULKAN_FUNCTION(name)                                                                           \
	name = std::bit_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name));                                          \
	if (!name)                                                                                                         \
	{                                                                                                                  \
		std::cout << "Could not load instance-level Vulkan function \"" #name "\"\n";                                  \
		return false;                                                                                                  \
	}

#define INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(name, extension)                                                 \
	for (auto& enabledExtension : extensions)                                                                          \
	{                                                                                                                  \
		if (std::string_view(enabledExtension) == std::string_view(extension))                                         \
		{                                                                                                              \
			name = std::bit_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name));                                  \
			if (!name)                                                                                                 \
			{                                                                                                          \
				std::cout << "Could not load instance-level Vulkan function \"" #name "\"\n";                          \
				return false;                                                                                          \
			}                                                                                                          \
			break;                                                                                                     \
		}                                                                                                              \
	}

#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION(name)                                                           \
	name = std::bit_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name));                                          \
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
			name = std::bit_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name));                                  \
			if (!name)                                                                                                 \
			{                                                                                                          \
				std::cout << "Could not load instance-level Vulkan function \"" #name "\"\n";                          \
				return false;                                                                                          \
			}                                                                                                          \
			break;                                                                                                     \
		}                                                                                                              \
	}

#define INSTANCE_LEVEL_DEVICE_VULKAN_FUNCTION(name)                                                                    \
	name = std::bit_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name));                                          \
	if (!name)                                                                                                         \
	{                                                                                                                  \
		std::cout << "Could not load instance-level Vulkan function \"" #name "\"\n";                                  \
		return false;                                                                                                  \
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

	bool physical_device::is_extension_available(std::string_view extensionName)
	{
		for (auto& extension : get_available_extensions())
		{
			if (extension.name == extensionName)
			{
				return true;
			}
		}

		return false;
	}

	namespace
	{
		queue_feature_flags map_vk_queue_features(VkQueueFlags features, bool supportsPresent)
		{
			return rsl::enum_flags::set_flag(
				static_cast<queue_feature_flags>(features), queue_feature_flags::present, supportsPresent
			);
		}
	} // namespace

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

			auto* nativeInstance = get_native_ptr(impl->instance);

			for (rsl::uint32 queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++)
			{
				auto& queueFamily = queueFamiliesBuffer[queueFamilyIndex];

				VkBool32 supportsPresent = VK_FALSE;

				if (nativeInstance->applicationInfo.windowHandle != invalid_native_window_handle)
				{
					VkSurfaceKHR surface = VK_NULL_HANDLE;

#if RYTHE_PLATFORM_WINDOWS
					VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
						.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
					};
					surfaceCreateInfo.hwnd = get_native_window(nativeInstance->applicationInfo.windowHandle);
					if (nativeInstance->vkCreateWin32SurfaceKHR(
							nativeInstance->instance, &surfaceCreateInfo, nullptr, &surface
						) != VK_SUCCESS)
					{
						surface = VK_NULL_HANDLE;
					}
#endif

					if (surface == VK_NULL_HANDLE ||
						nativeInstance->vkGetPhysicalDeviceSurfaceSupportKHR(
							impl->physicalDevice, queueFamilyIndex, surface, &supportsPresent
						) != VK_SUCCESS)
					{
						supportsPresent = VK_FALSE;
					}
				}

				impl->availableQueueFamilies.push_back({
					.features = map_vk_queue_features(queueFamily.queueFlags, supportsPresent == VK_TRUE),
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

	bool physical_device::get_queue_family_selection(
		std::span<queue_family_selection> queueFamilySelections, std::span<const queue_description> queueDesciptions
	)
	{
		for (rsl::size_type i = 0; i < queueDesciptions.size(); i++)
		{
			queueFamilySelections[i].familyIndex = queueDesciptions[i].queueFamilyIndexOverride;
		}

		auto queueFamilies = get_available_queue_families();

		for (rsl::size_type queueIndex = 0; queueIndex < queueDesciptions.size(); queueIndex++)
		{
			auto& queueDesciption = queueDesciptions[queueIndex];
			auto& selectionInfo = queueFamilySelections[queueIndex];

			selectionInfo.familyIndex = rsl::npos;
			for (rsl::size_type familyIndex = 0; familyIndex < queueFamilies.size(); familyIndex++)
			{
				auto& queueFamily = queueFamilies[familyIndex];

				if (queueDesciption.queueFamilyIndexOverride != rsl::npos ||
					!rsl::enum_flags::has_all_flags(queueFamily.features, queueDesciption.requiredFeatures) ||
					queueFamily.queueCount == 0)
				{
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

				if (score > selectionInfo.score)
				{
					selectionInfo.familyIndex = familyIndex;
					selectionInfo.score = score;
				}
			}

			if (selectionInfo.familyIndex == rsl::npos)
			{
				std::cout << "No compatible queue family found for queue " << queueIndex << '\n';
				return false;
			}
		}

		return true;
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
				case VK_PHYSICAL_DEVICE_TYPE_OTHER: return physical_device_type::other;
				case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return physical_device_type::integratedGPU;
				case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return physical_device_type::discreteGPU;
				case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return physical_device_type::virtualGPU;
				case VK_PHYSICAL_DEVICE_TYPE_CPU: return physical_device_type::CPU;
				case VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM: return physical_device_type::other;
			}

			return physical_device_type::other;
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

	void physical_device::release()
	{
		auto* impl = get_native_ptr(*this);

		if (!impl)
		{
			return;
		}

		m_nativePhysicalDevice = invalid_native_physical_device;
		delete impl;
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

	bool physical_device::in_use() const noexcept
	{
		return get_native_ptr(*this)->renderDevice;
	}

	render_device physical_device::create_render_device(
		std::span<const queue_description> queueDesciptions, std::span<rsl::cstring> extensions
	)
	{
		for (auto& extensionName : extensions)
		{
			if (!is_extension_available(extensionName))
			{
				std::cout << "Extension \"" << extensionName << "\" is not available.\n";
				return {};
			}
		}

		return create_render_device_no_extension_check(*this, queueDesciptions, extensions);
	}

	render_device::operator bool() const noexcept
	{
		auto ptr = get_native_ptr(*this);
		return ptr != nullptr && ptr->device != VK_NULL_HANDLE;
	}

	void render_device::release()
	{
		auto* impl = get_native_ptr(*this);

		if (!impl)
		{
			return;
		}

		impl->vkDestroyDevice(impl->device, nullptr);

		impl->physicalDevice.release();

		m_nativeRenderDevice = invalid_native_render_device;
		delete impl;
	}

	std::span<queue> render_device::get_queues() noexcept
	{
		auto* impl = get_native_ptr(*this);

		if (impl)
		{
			return impl->queues;
		}

		return {};
	}

	physical_device render_device::get_physical_device() const noexcept
	{
		auto ptr = get_native_ptr(*this);
		if (!ptr)
		{
			return physical_device();
		}

		return ptr->physicalDevice;
	}

	bool native_render_device_vk::load_functions(std::span<rsl::cstring> extensions)
	{
#define DEVICE_LEVEL_VULKAN_FUNCTION(name)                                                                             \
	name = std::bit_cast<PFN_##name>(vkGetDeviceProcAddr(device, #name));                                              \
	if (!name)                                                                                                         \
	{                                                                                                                  \
		std::cout << "Could not load device-level Vulkan function \"" #name "\"\n";                                    \
		return false;                                                                                                  \
	}

#define DEVICE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(name, extension)                                                   \
	for (auto& enabledExtension : extensions)                                                                          \
	{                                                                                                                  \
		if (std::string_view(enabledExtension) == std::string_view(extension))                                         \
		{                                                                                                              \
			name = std::bit_cast<PFN_##name>(vkGetDeviceProcAddr(device, #name));                                      \
			if (!name)                                                                                                 \
			{                                                                                                          \
				std::cout << "Could not load device-level Vulkan function \"" #name "\"\n";                            \
				return false;                                                                                          \
			}                                                                                                          \
			break;                                                                                                     \
		}                                                                                                              \
	}

#include "impl/list_of_vulkan_functions.inl"

		return true;
	}

	rsl::size_type queue::get_index() const noexcept
	{
		auto* impl = get_native_ptr(*this);

		if (impl)
		{
			return impl->queueIndex;
		}

		return rsl::npos;
	}

	rsl::size_type queue::get_family_index() const noexcept
	{
		auto* impl = get_native_ptr(*this);

		if (impl)
		{
			return impl->familyIndex;
		}

		return rsl::npos;
	}

	queue_priority queue::get_priority() const noexcept
	{
		auto* impl = get_native_ptr(*this);

		if (impl)
		{
			return impl->priority;
		}

		return queue_priority::normal;
	}

} // namespace vk
