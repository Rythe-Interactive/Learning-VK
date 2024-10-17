#pragma once

#include <rsl/platform>
#include <rsl/math>

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

#include <semver/semver.hpp>
#include <span>

namespace vk
{
	bool init();

	struct extension_properties
	{
		std::string name;
		semver::version specVersion;
	};

	std::span<const extension_properties> get_available_instance_extensions(bool forceRefresh = false);

	bool is_instance_extension_available(std::string_view extensionName);

	struct application_info
	{
		std::string name;
		semver::version version;
	};

	class instance;

	instance create_instance(
		const application_info& applicationInfo, const semver::version& apiVersion = {1, 0, 0},
		std::span<const char*> extensions = {}
	);

	class physical_device;

	class instance
	{
	public:
		operator bool() const { return m_instance != VK_NULL_HANDLE; }

		std::span<physical_device> get_physical_devices(bool forceRefresh = false);

	private:
		bool load_functions(std::span<const char*> extensions);

#define INSTANCE_LEVEL_VULKAN_FUNCTION(name) [[maybe_unused]] PFN_##name name;
#define INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(name, extension) [[maybe_unused]] PFN_##name name;
#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION(name) [[maybe_unused]] PFN_##name name;
#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION_FROM_EXTENSION(name, extension) [[maybe_unused]] PFN_##name name;
#define INSTANCE_LEVEL_DEVICE_VULKAN_FUNCTION(name) [[maybe_unused]] PFN_##name name;
#include "impl/list_of_vulkan_functions.inl"

		std::vector<physical_device> m_physicalDevices;

		VkInstance m_instance = VK_NULL_HANDLE;

		friend instance create_instance(
			const application_info& applicationInfo, const semver::version& apiVersion,
			std::span<const char*> extensions
		);
	};

	using physical_device_features = VkPhysicalDeviceFeatures;

	struct physical_device_properties
	{
		semver::version apiVersion;
		semver::version driverVersion;
		rsl::uint32 vendorID;
		rsl::uint32 deviceID;
		VkPhysicalDeviceType deviceType;
		std::string deviceName;
		rsl::uint8 pipelineCacheUUID[VK_UUID_SIZE];
		VkPhysicalDeviceLimits limits;
		VkPhysicalDeviceSparseProperties sparseProperties;
	};

	enum struct queue_feature_flags
	{
		GRAPHICS = 0x00000001,
		COMPUTE = 0x00000002,
		TRANSFER = 0x00000004,
		SPARSE_BINDING = 0x00000008,
		PROTECTED = 0x00000010,
		VIDEO_DECODE = 0x00000020,
		VIDEO_ENCODE = 0x00000040,
		OPTICAL_FLOW_NV = 0x00000100,
	};

	struct queue_family_properties
	{
		queue_feature_flags features;
		rsl::uint32 queueCount;
		rsl::uint32 timestampValidBits;
		rsl::math::uint3 minImageTransferGranularity;
	};

	class physical_device
	{
	public:
		operator bool() const { return m_physicalDevice != VK_NULL_HANDLE; }

		const physical_device_properties& get_properties(bool forceRefresh = false);
		const physical_device_features& get_features(bool forceRefresh = false);
		std::span<const extension_properties> get_available_extensions(bool forceRefresh = false);
		std::span<const queue_family_properties> get_available_queue_families(bool forceRefresh = false);

		bool initialize(std::span<const char*> extensions);

	private:
		bool load_functions(std::span<const char*> extensions);

#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION(name) [[maybe_unused]] PFN_##name name;
#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION_FROM_EXTENSION(name, extension) [[maybe_unused]] PFN_##name name;
#include "impl/list_of_vulkan_functions.inl"

		bool m_featuresLoaded = false;
		physical_device_features m_features;
		bool m_propertiesLoaded = false;
		physical_device_properties m_properties;
		std::vector<extension_properties> m_availableExtensions;
		std::vector<queue_family_properties> m_availableQueueFamilies;

		VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
		friend class instance;
	};

	std::string_view to_string(VkPhysicalDeviceType type);

	class render_device
	{

	};
} // namespace vk
