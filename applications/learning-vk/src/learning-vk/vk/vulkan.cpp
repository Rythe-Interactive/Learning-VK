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
	#elif RYTHE_SURFACE_XLIB
		#define VK_USE_PLATFORM_XLIB_KHR
		#include <vulkan/vulkan_xlib.h>
		#define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_XLIB_SURFACE_EXTENSION_NAME
	#endif
#endif

#define MAKE_HASHED_STRING_VIEW_LITERAL_IMPL(str) (str##_hsv)
#define MAKE_HASHED_STRING_VIEW_LITERAL(str) MAKE_HASHED_STRING_VIEW_LITERAL_IMPL(str)

namespace vk
{
	namespace
	{
		[[rythe_always_inline]] constexpr semver::version decomposeVkVersion(rsl::uint32 vkVersion) noexcept
		{
			return semver::version{
				static_cast<rsl::uint8>(VK_API_VERSION_MAJOR(vkVersion)),
				static_cast<rsl::uint8>(VK_API_VERSION_MINOR(vkVersion)),
				static_cast<rsl::uint8>(VK_API_VERSION_PATCH(vkVersion)),
			};
		}

		[[nodiscard]] [[rythe_allocating]] void* defaultAllocFunc(rsl::size_type size, void*) noexcept
		{
			void* mem = ::operator new(size);
			return mem;
		}

		[[nodiscard]] [[rythe_allocating]] void*
		defaultAllocAlignedFunc(rsl::size_type size, rsl::size_type alignment, void*) noexcept
		{
			void* mem = ::operator new(size, std::align_val_t{alignment});
			return mem;
		}

		[[nodiscard]] [[rythe_allocating]] void*
		defaultReallocFunc(void* ptr, rsl::size_type oldSize, rsl::size_type newSize, rsl::size_type alignment, void*) noexcept
		{
			void* mem = nullptr;

			if (newSize != 0)
			{
				if (alignment != 0)
				{
					mem = ::operator new(newSize, std::align_val_t{alignment});
				}
				else
				{
					mem = ::operator new(newSize);
				}

				std::memcpy(mem, ptr, std::min(oldSize, newSize));
			}

			if (alignment != 0)
			{
				::operator delete(ptr, oldSize, std::align_val_t{alignment});
			}
			else
			{
				::operator delete(ptr, oldSize);
			}

			return mem;
		}

		void defaultDeallocFunc(void* ptr, rsl::size_type size, void*) noexcept
		{
			::operator delete(ptr, size);
		}

		void defaultDeallocAlignedFunc(void* ptr, rsl::size_type size, rsl::size_type alignment, void*) noexcept
		{
			::operator delete(ptr, size, std::align_val_t{alignment});
		}

		void validateAllocator(allocator& alloc) noexcept
		{
			if (!alloc.allocFunc || !alloc.deallocFunc || !alloc.alignedAllocFunc || !alloc.reallocFunc ||
				!alloc.alignedDeallocFunc)
			{
				alloc.allocFunc = &defaultAllocFunc;
				alloc.alignedAllocFunc = &defaultAllocAlignedFunc;
				alloc.reallocFunc = &defaultReallocFunc;
				alloc.deallocFunc = &defaultDeallocFunc;
				alloc.alignedDeallocFunc = &defaultDeallocAlignedFunc;
				alloc.userData = nullptr;
			}
		}

		template <typename T, typename... Args>
		[[nodiscard]] T* allocate(
			allocator& alloc, rsl::size_type count = 1, Args&&... args
		) noexcept(std::is_nothrow_constructible_v<T, Args...>)
		{
			T* mem = static_cast<T*>(alloc.allocFunc(sizeof(T) * count, alloc.userData));

			if constexpr (std::is_trivially_constructible_v<T> && sizeof...(Args) == 0)
			{
				std::memset(mem, 0, sizeof(T) * count);
			}
			else
			{
				for (rsl::size_type i = 0; i < count; i++) { new (&mem[i]) T(std::forward<Args>(args)...); }
			}

			return mem;
		}

		template <typename T>
		void deallocate(allocator& alloc, T* ptr, rsl::size_type count = 1) noexcept
		{
			if constexpr (!std::is_trivially_destructible_v<T>)
			{
				for (rsl::size_type i = 0; i < count; i++) { ptr[i].~T(); }
			}

			alloc.deallocFunc(ptr, sizeof(T) * count, alloc.userData);
		}

	} // namespace

#if RYTHE_PLATFORM_WINDOWS
	[[nodiscard]] native_window_handle create_window_handle_win32(const native_window_info_win32& windowInfo)
	{
		allocator alloc = windowInfo.alloc;
		validateAllocator(alloc);

		auto* ptr = allocate<native_window_info_win32>(alloc, 1, windowInfo);
		ptr->alloc = alloc;
		return std::bit_cast<native_window_handle>(ptr);
	}

	namespace
	{
		HWND get_hwnd(native_window_handle handle)
		{
			return std::bit_cast<native_window_info_win32*>(handle)->hwnd;
		}

		HINSTANCE get_hinstance(native_window_handle handle)
		{
			return std::bit_cast<native_window_info_win32*>(handle)->hinstance;
		}
	} // namespace
#elif RYTHE_PLATFORM_LINUX
	#ifdef RYTHE_SURFACE_XCB
	[[nodiscard]] native_window_handle create_window_handle_xcb(const native_window_info_xcb& windowInfo)
	{
		allocator alloc = windowInfo.alloc;
		validateAllocator(alloc);

		auto* ptr = allocate<native_window_info_xcb>(alloc, 1, windowInfo);
		ptr->alloc = alloc;
		return std::bit_cast<native_window_handle>(ptr);
	}

	namespace
	{
		xcb_connection_t* get_connection(native_window_handle handle)
		{
			return std::bit_cast<native_window_info_xcb*>(handle)->connection;
		}

		xcb_window_t get_window(native_window_handle handle)
		{
			return std::bit_cast<native_window_info_xcb*>(handle)->window;
		}
	} // namespace
	#elif RYTHE_SURFACE_XLIB
	[[nodiscard]] native_window_handle create_window_handle_xlib(const native_window_info_xlib& windowInfo)
	{
		allocator alloc = windowInfo.alloc;
		validateAllocator(alloc);

		auto* ptr = allocate<native_window_info_xlib>(alloc, 1, windowInfo);
		ptr->alloc = alloc;
		return std::bit_cast<native_window_handle>(ptr);
	}

	namespace
	{
		Display* get_display(native_window_handle handle)
		{
			return std::bit_cast<native_window_info_xlib*>(handle)->display;
		}

		Window get_window(native_window_handle handle)
		{
			return std::bit_cast<native_window_info_xlib*>(handle)->window;
		}
	} // namespace
	#endif
#endif

	void release_window_handle(native_window_handle handle)
	{
#if RYTHE_PLATFORM_WINDOWS
		auto* ptr = std::bit_cast<native_window_info_win32*>(handle);
		deallocate<native_window_info_win32>(ptr->alloc, ptr);
#elif RYTHE_PLATFORM_LINUX
	#ifdef RYTHE_SURFACE_XCB
		auto* ptr = std::bit_cast<native_window_info_xcb*>(handle);
		deallocate<native_window_info_xcb>(ptr->alloc, ptr);
	#elif RYTHE_SURFACE_XLIB
		auto* ptr = std::bit_cast<native_window_info_xlib*>(handle);
		deallocate<native_window_info_xlib>(ptr->alloc, ptr);
	#endif
#endif
	}

	static void set_native_handle(graphics_library& target, native_graphics_library handle)
	{
		target.m_nativeGL = handle;
	}

	static void set_native_handle(instance& target, native_instance handle)
	{
		target.m_nativeInstance = handle;
	}

	static void set_native_handle(surface& target, native_surface handle)
	{
		target.m_nativeSurface = handle;
	}

	static void set_native_handle(physical_device& target, native_physical_device handle)
	{
		target.m_nativePhysicalDevice = handle;
	}

	static void set_native_handle(render_device& target, native_render_device handle)
	{
		target.m_nativeRenderDevice = handle;
	}

	static void set_native_handle(queue& target, native_queue handle)
	{
		target.m_nativeQueue = handle;
	}

	static void set_native_handle(command_pool& target, native_command_pool handle)
	{
		target.m_nativeCommandPool = handle;
	}

	static void set_native_handle(command_buffer& target, native_command_buffer handle)
	{
		target.m_nativeCommandBuffer = handle;
	}

	namespace
	{
		template <typename T>
		struct native_handle_traits
		{
		};

		struct native_graphics_library_vk
		{
			rsl::dynamic_library vulkanLibrary;
			std::vector<layer_properties> availableInstanceLayers;
			std::vector<extension_properties> availableInstanceExtensions;

			allocator alloc;
			VkAllocationCallbacks allocCallbacks;

#define EXPORTED_VULKAN_FUNCTION(name) PFN_##name name = nullptr;
#define GLOBAL_LEVEL_VULKAN_FUNCTION(name) PFN_##name name = nullptr;
#include "impl/list_of_vulkan_functions.inl"

			constexpr static rsl::platform_dependent_var vulkanLibName = {
				rsl::windows_var{"vulkan-1.dll"},
				rsl::linux_var{"libvulkan.so.1"},
			};
		};

		template <>
		struct native_handle_traits<graphics_library>
		{
			using native_type = native_graphics_library_vk;
			using handle_type = native_graphics_library;
		};

		template <>
		struct native_handle_traits<native_graphics_library_vk>
		{
			using api_type = graphics_library;
			using handle_type = native_graphics_library;
		};

		struct native_instance_vk
		{
			bool load_functions(std::span<const rsl::cstring> extensions);

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
			allocator* alloc = nullptr;
			VkAllocationCallbacks* allocCallbacks = nullptr;

			std::vector<layer_properties> enabledLayers;
			std::vector<extension_properties> enabledExtensions;

			VkInstance instance = VK_NULL_HANDLE;
			graphics_library graphicsLib;
		};

		template <>
		struct native_handle_traits<vk::instance>
		{
			using native_type = native_instance_vk;
			using handle_type = native_instance;
		};

		template <>
		struct native_handle_traits<native_instance_vk>
		{
			using api_type = vk::instance;
			using handle_type = native_instance;
		};

		struct native_surface_vk
		{
			VkSurfaceKHR surface = VK_NULL_HANDLE;

			allocator* alloc = nullptr;
			VkAllocationCallbacks* allocCallbacks = nullptr;

			instance instance;
		};

		template <>
		struct native_handle_traits<surface>
		{
			using native_type = native_surface_vk;
			using handle_type = native_surface;
		};

		template <>
		struct native_handle_traits<native_surface_vk>
		{
			using api_type = surface;
			using handle_type = native_surface;
		};

		struct native_physical_device_vk
		{
#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION(name) [[maybe_unused]] PFN_##name name = nullptr;
#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION_FROM_EXTENSION(name, extension)                                 \
	[[maybe_unused]] PFN_##name name = nullptr;
#define INSTANCE_LEVEL_DEVICE_VULKAN_FUNCTION(name) [[maybe_unused]] PFN_##name name = nullptr;
#include "impl/list_of_vulkan_functions.inl"

			render_device renderDevice;

			bool surfaceCapsLoaded = false;
			surface_capabilities surfaceCaps;
			bool featuresLoaded = false;
			physical_device_features features;
			bool propertiesLoaded = false;
			physical_device_properties properties;
			std::vector<layer_properties> availableLayers;
			std::vector<extension_properties> availableExtensions;
			std::vector<queue_family_properties> availableQueueFamilies;

			allocator* alloc = nullptr;
			VkAllocationCallbacks* allocCallbacks = nullptr;
			instance instance;

			VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		};

		template <>
		struct native_handle_traits<physical_device>
		{
			using native_type = native_physical_device_vk;
			using handle_type = native_physical_device;
		};

		template <>
		struct native_handle_traits<native_physical_device_vk>
		{
			using api_type = physical_device;
			using handle_type = native_physical_device;
		};

		struct native_render_device_vk
		{
			bool load_functions(std::span<const rsl::cstring> extensions);

#define INSTANCE_LEVEL_DEVICE_VULKAN_FUNCTION(name) [[maybe_unused]] PFN_##name name = nullptr;
#define DEVICE_LEVEL_VULKAN_FUNCTION(name) [[maybe_unused]] PFN_##name name = nullptr;
#define DEVICE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(name, extension) [[maybe_unused]] PFN_##name name = nullptr;
#include "impl/list_of_vulkan_functions.inl"

			physical_device physicalDevice;
			allocator* alloc = nullptr;
			VkAllocationCallbacks* allocCallbacks = nullptr;

			std::vector<queue> queues;

			VkDevice device = VK_NULL_HANDLE;
		};

		template <>
		struct native_handle_traits<render_device>
		{
			using native_type = native_render_device_vk;
			using handle_type = native_render_device;
		};

		template <>
		struct native_handle_traits<native_render_device_vk>
		{
			using api_type = render_device;
			using handle_type = native_render_device;
		};

		struct native_queue_vk
		{
			render_device renderDevice;
			allocator* alloc = nullptr;
			VkAllocationCallbacks* allocCallbacks = nullptr;

			rsl::size_type queueIndex;
			rsl::size_type familyIndex;
			queue_family_properties family;
			queue_priority priority;
			VkQueue queue = VK_NULL_HANDLE;
		};

		template <>
		struct native_handle_traits<queue>
		{
			using native_type = native_queue_vk;
			using handle_type = native_queue;
		};

		template <>
		struct native_handle_traits<native_queue_vk>
		{
			using api_type = queue;
			using handle_type = native_queue;
		};

		struct native_command_pool_vk
		{
			render_device renderDevice;
			allocator* alloc = nullptr;
			VkAllocationCallbacks* allocCallbacks = nullptr;

			std::vector<VkCommandBuffer> commandBuffersBuffer;

			struct commandBufferPool
			{
				rsl::size_type lastUnusedIndex = 0ull;
				rsl::size_type unusedCount = 0ull;
				std::vector<command_buffer> commandBuffers;
			};

			commandBufferPool primaryCommandBuffers;
			commandBufferPool secondaryCommandBuffers;

			VkCommandPool commandPool = VK_NULL_HANDLE;
		};

		template <>
		struct native_handle_traits<command_pool>
		{
			using native_type = native_command_pool_vk;
			using handle_type = native_command_pool;
		};

		template <>
		struct native_handle_traits<persistent_command_pool>
		{
			using native_type = native_command_pool_vk;
			using handle_type = native_command_pool;
		};

		template <>
		struct native_handle_traits<transient_command_pool>
		{
			using native_type = native_command_pool_vk;
			using handle_type = native_command_pool;
		};

		template <>
		struct native_handle_traits<native_command_pool_vk>
		{
			using api_type = command_pool;
			using handle_type = native_command_pool;
		};

		namespace
		{
			struct non_descript_command_pool
			{
				union
				{
					persistent_command_pool persistent;
					transient_command_pool transient;
				} data = {.persistent = {}};

				bool isPersistent = true;

				void set(persistent_command_pool pool)
				{
					data.persistent = pool;
					isPersistent = true;
				}
				void set(transient_command_pool pool)
				{
					data.transient = pool;
					isPersistent = false;
				}

				const command_pool& get_pool() const
				{
					return *(
						isPersistent ? static_cast<const command_pool*>(&data.persistent)
									 : static_cast<const command_pool*>(&data.transient)
					);
				}

				command_pool& get_pool()
				{
					return *(
						isPersistent ? static_cast<command_pool*>(&data.persistent)
									 : static_cast<command_pool*>(&data.transient)
					);
				}
			};
		} // namespace

		struct native_command_buffer_vk
		{
			render_device device;

			non_descript_command_pool commandPoolStorage;

			const command_pool& get_pool() const { return commandPoolStorage.get_pool(); }
			command_pool& get_pool() { return commandPoolStorage.get_pool(); }

			rsl::size_type nextUnusedIndex = rsl::npos;
			rsl::size_type indexInPool = rsl::npos;
			command_buffer_level level;

			VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
		};

		template <>
		struct native_handle_traits<command_buffer>
		{
			using native_type = native_command_buffer_vk;
			using handle_type = native_command_buffer;
		};

		template <>
		struct native_handle_traits<native_command_buffer_vk>
		{
			using api_type = command_buffer;
			using handle_type = native_command_buffer;
		};

		template <typename T>
		[[nodiscard]] [[rythe_always_inline]] typename native_handle_traits<T>::native_type*
		get_native_ptr(const T& inst)
		{
			return std::bit_cast<typename native_handle_traits<T>::native_type*>(inst.get_native_handle());
		}

		template <typename T>
		[[nodiscard]] [[rythe_always_inline]] typename native_handle_traits<T>::native_type&
		get_native_ref(const T& inst)
		{
			auto* ptr = get_native_ptr(inst);
			rsl_assert_invalid_object(ptr);
			return *ptr;
		}

		template <typename T>
		[[nodiscard]] [[rythe_always_inline]] auto create_native_handle(T* inst)
		{
			return std::bit_cast<typename native_handle_traits<T>::handle_type>(inst);
		}

		struct alloc_data
		{
			rsl::size_type size;
			rsl::size_type alignment;
		};

		rsl::size_type get_additional_alloc_size(rsl::size_type alignment)
		{
			if (alignment >= sizeof(alloc_data))
			{
				return alignment;
			}

            return sizeof(alloc_data);
		}

		void* defaultVKAllocFunc(
			void* userData, rsl::size_type size, rsl::size_type alignment, VkSystemAllocationScope
		) noexcept
		{
			allocator* alloc = static_cast<allocator*>(userData);

            const rsl::size_type additionalAllocSize = get_additional_alloc_size(alignment);
            const rsl::size_type totalSize = size + additionalAllocSize;
			void* mem = alloc->alignedAllocFunc(totalSize, alignment, alloc->userData);

			alloc_data* allocData = std::bit_cast<alloc_data*>(static_cast<rsl::byte*>(mem) + additionalAllocSize) - 1;

            allocData->alignment = alignment;
			allocData->size = totalSize;

			return static_cast<rsl::byte*>(mem) + additionalAllocSize;
		}

		void defaultVKFreeFunc(void* userData, void* ptr) noexcept
		{
			if (!ptr)
			{
				return;
			}

			allocator* alloc = static_cast<allocator*>(userData);
			alloc_data* allocData = static_cast<alloc_data*>(ptr) - 1;

			const rsl::size_type additionalAllocSize = get_additional_alloc_size(allocData->alignment);
			void* originalPtr = static_cast<void*>(static_cast<rsl::byte*>(ptr) - additionalAllocSize);

			alloc->alignedDeallocFunc(originalPtr, allocData->size, allocData->alignment, alloc->userData);
		}

		void* defaultVKReallocationFunc(
			void* userData, void* ptr, rsl::size_type size, rsl::size_type alignment, VkSystemAllocationScope
		) noexcept
		{
			allocator* alloc = static_cast<allocator*>(userData);
			alloc_data* oldAllocData = static_cast<alloc_data*>(ptr) - 1;

            rsl_assert_msg_consistent(alignment == oldAllocData->alignment, "alignment mismatch");

			void* originalPtr =
				static_cast<void*>(static_cast<rsl::byte*>(ptr) - get_additional_alloc_size(oldAllocData->alignment));

			const rsl::size_type additionalAllocSize = get_additional_alloc_size(alignment);
			const rsl::size_type totalSize = size + additionalAllocSize;
			void* mem = alloc->reallocFunc(originalPtr, oldAllocData->size, totalSize,
				alignment, alloc->userData
			);
            
			alloc_data* newAllocData = std::bit_cast<alloc_data*>(static_cast<rsl::byte*>(mem) + additionalAllocSize) - 1;
			newAllocData->alignment = alignment;
			newAllocData->size = totalSize;

			return static_cast<rsl::byte*>(mem) + additionalAllocSize;
		}

		VkAllocationCallbacks createVKAllocator(allocator* alloc) noexcept
		{
			return VkAllocationCallbacks{
				.pUserData = alloc,
				.pfnAllocation = &defaultVKAllocFunc,
				.pfnReallocation = &defaultVKReallocationFunc,
				.pfnFree = &defaultVKFreeFunc,
				.pfnInternalAllocation = nullptr,
				.pfnInternalFree = nullptr,
			};
		}
	} // namespace

	[[nodiscard]] graphics_library init(allocator alloc)
	{
		validateAllocator(alloc);

		native_graphics_library_vk* nativeGL = allocate<native_graphics_library_vk>(alloc);

		nativeGL->alloc = alloc;
		nativeGL->allocCallbacks = createVKAllocator(&nativeGL->alloc);
		nativeGL->vulkanLibrary = rsl::platform::load_library(native_graphics_library_vk::vulkanLibName);

		if (!nativeGL->vulkanLibrary)
		{
			std::cout << "could not load " << native_graphics_library_vk::vulkanLibName.get() << '\n';
			deallocate<native_graphics_library_vk>(alloc, nativeGL);
			return {};
		}

#define EXPORTED_VULKAN_FUNCTION(name)                                                                                 \
	nativeGL->name = nativeGL->vulkanLibrary.get_symbol<PFN_##name>(#name);                                            \
	if (!nativeGL->name)                                                                                               \
	{                                                                                                                  \
		std::cout << "Could not load exported Vulkan function \"" #name "\"\n";                                        \
		deallocate<native_graphics_library_vk>(alloc, nativeGL);                                                       \
		return {};                                                                                                     \
	}

#define GLOBAL_LEVEL_VULKAN_FUNCTION(name)                                                                             \
	nativeGL->name = std::bit_cast<PFN_##name>(nativeGL->vkGetInstanceProcAddr(nullptr, #name));                       \
	if (!nativeGL->name)                                                                                               \
	{                                                                                                                  \
		std::cout << "Could not load global-level Vulkan function \"" #name "\"\n";                                    \
		deallocate<native_graphics_library_vk>(alloc, nativeGL);                                                       \
		return {};                                                                                                     \
	}

#include "impl/list_of_vulkan_functions.inl"

		graphics_library result;
		set_native_handle(result, create_native_handle(nativeGL));

		return result;
	}

	graphics_library::operator bool() const noexcept
	{
		return get_native_ptr(*this);
	}

	void graphics_library::release()
	{
		auto* impl = get_native_ptr(*this);
		if (!impl)
		{
			return;
		}

		impl->vulkanLibrary.release();

		m_nativeGL = invalid_native_graphics_library;
		deallocate<native_graphics_library_vk>(impl->alloc, impl);
	}

	namespace
	{
		[[rythe_always_inline]] constexpr rsl::size_type
		get_layer_index(std::span<const layer_properties> layers, rsl::hashed_string_view layerName) noexcept
		{
			for (rsl::size_type i = 0; i < layers.size(); i++)
			{
				if (layers[i].name == layerName)
				{
					return i;
				}
			}
			return rsl::npos;
		}

		[[rythe_always_inline]] constexpr rsl::size_type get_extension_index(
			std::span<const extension_properties> extensions, rsl::hashed_string_view extensionName
		) noexcept
		{
			for (rsl::size_type i = 0; i < extensions.size(); i++)
			{
				if (extensions[i].name == extensionName)
				{
					return i;
				}
			}
			return rsl::npos;
		}
	} // namespace

	std::span<const layer_properties> graphics_library::get_available_instance_layers(bool forceRefresh)
	{
		auto& impl = get_native_ref(*this);

		if (forceRefresh || impl.availableInstanceLayers.empty())
		{
			rsl::uint32 layerCount = 0;
			VkResult result = impl.vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
			if (result != VK_SUCCESS || layerCount == 0)
			{
				std::cout << "Could not query the number of instance layers.\n";
				return {};
			}

			std::vector<VkLayerProperties> availableInstanceLayersBuffer;
			availableInstanceLayersBuffer.resize(layerCount);
			result = impl.vkEnumerateInstanceLayerProperties(&layerCount, availableInstanceLayersBuffer.data());
			if (result != VK_SUCCESS || layerCount == 0)
			{
				std::cout << "Could not enumerate instance layers.\n";
				return {};
			}

			impl.availableInstanceLayers.clear();
			impl.availableInstanceLayers.reserve(layerCount);
			for (auto& layer : availableInstanceLayersBuffer)
			{
				impl.availableInstanceLayers.push_back({
					.name = rsl::hashed_string(layer.layerName),
					.specVersion = decomposeVkVersion(layer.specVersion),
					.implementationVersion = decomposeVkVersion(layer.implementationVersion),
					.description = std::string(layer.description),
				});
			}
		}

		return impl.availableInstanceLayers;
	}

	bool graphics_library::is_instance_layer_available(rsl::hashed_string_view layerName)
	{
		return get_layer_index(get_available_instance_layers(), layerName) != rsl::npos;
	}

	std::span<const extension_properties> graphics_library::get_available_instance_extensions(bool forceRefresh)
	{
		auto& impl = get_native_ref(*this);

		if (forceRefresh || impl.availableInstanceExtensions.empty())
		{
			rsl::uint32 extensionCount = 0;
			VkResult result = impl.vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
			if (result != VK_SUCCESS || extensionCount == 0)
			{
				std::cout << "Could not query the number of instance extensions.\n";
				return {};
			}

			std::vector<VkExtensionProperties> availableInstanceExtensionsBuffer;
			availableInstanceExtensionsBuffer.resize(extensionCount);
			result = impl.vkEnumerateInstanceExtensionProperties(
				nullptr, &extensionCount, availableInstanceExtensionsBuffer.data()
			);
			if (result != VK_SUCCESS || extensionCount == 0)
			{
				std::cout << "Could not enumerate instance extensions.\n";
				return {};
			}

			impl.availableInstanceExtensions.clear();
			impl.availableInstanceExtensions.reserve(extensionCount);
			for (auto& extension : availableInstanceExtensionsBuffer)
			{
				impl.availableInstanceExtensions.push_back({
					.name = rsl::hashed_string(extension.extensionName),
					.specVersion = decomposeVkVersion(extension.specVersion),
				});
			}
		}

		return impl.availableInstanceExtensions;
	}

	bool graphics_library::is_instance_extension_available(rsl::hashed_string_view extensionName)
	{
		return get_extension_index(get_available_instance_extensions(), extensionName) != rsl::npos;
	}

	[[nodiscard]] instance graphics_library::create_instance(
		const application_info& _applicationInfo, const semver::version& apiVersion,
		std::span<const rsl::hashed_string> layers, std::span<const rsl::hashed_string> extensions
	)
	{
		using namespace rsl::hashed_string_literals;

		std::vector<layer_properties> enabledLayerProperties;
		std::vector<rsl::cstring> enabledLayers;
		enabledLayerProperties.reserve(layers.size());
		enabledLayers.reserve(layers.size());

#ifdef RYTHE_DEBUG
		bool khrValidationLayerActive = false;
#endif // RYTHE_DEBUG

		auto availableLayers = get_available_instance_layers();
		for (auto& layerName : layers)
		{
			rsl::size_type layerIndex = get_layer_index(availableLayers, layerName);
			if (layerIndex == rsl::npos)
			{
				std::cout << "Layer \"" << layerName.c_str() << "\" is not available.\n";
			}
			else
			{
#ifdef RYTHE_DEBUG
				if (layerName == "VK_LAYER_KHRONOS_validation"_hsv)
				{
					khrValidationLayerActive = true;
				}
#endif // RYTHE_DEBUG

				enabledLayerProperties.push_back(availableLayers[layerIndex]);
				enabledLayers.push_back(layerName.c_str());
			}
		}

#ifdef RYTHE_DEBUG
		if (!khrValidationLayerActive)
		{
			rsl::size_type validationLayerIndex = get_layer_index(availableLayers, "VK_LAYER_KHRONOS_validation"_hsv);
			if (validationLayerIndex != rsl::npos)
			{
				enabledLayerProperties.push_back(availableLayers[validationLayerIndex]);
				enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
			}
		}
#endif // RYTHE_DEBUG

		std::vector<extension_properties> enabledExtensionProperties;
		std::vector<rsl::cstring> enabledExtensions;
		enabledExtensionProperties.reserve(extensions.size());
		enabledExtensions.reserve(extensions.size());

		bool surfaceExtensionActive = false;
		bool platformSurfaceExtensionActive = false;
#ifdef RYTHE_DEBUG
		bool debugUtilsExtensionActive = false;
#endif // RYTHE_DEBUG

		auto availableExtensions = get_available_instance_extensions();
		for (auto& extensionName : extensions)
		{
			rsl::size_type extensionIndex = get_extension_index(availableExtensions, extensionName);
			if (extensionIndex == rsl::npos)
			{
				std::cout << "Extension \"" << extensionName.c_str() << "\" is not available.\n";
			}
			else
			{
				if (extensionName == MAKE_HASHED_STRING_VIEW_LITERAL(VK_KHR_SURFACE_EXTENSION_NAME))
				{
					surfaceExtensionActive = true;
				}
				else if (extensionName == MAKE_HASHED_STRING_VIEW_LITERAL(VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME))
				{
					platformSurfaceExtensionActive = true;
				}
#ifdef RYTHE_DEBUG
				else if (extensionName == MAKE_HASHED_STRING_VIEW_LITERAL(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
				{
					debugUtilsExtensionActive = true;
				}
#endif // RYTHE_DEBUG

				enabledExtensionProperties.push_back(availableExtensions[extensionIndex]);
				enabledExtensions.push_back(extensionName.c_str());
			}
		}

#ifdef RYTHE_DEBUG
		if (!debugUtilsExtensionActive)
		{
			enabledExtensionProperties.push_back(availableExtensions[get_extension_index(
				availableExtensions, MAKE_HASHED_STRING_VIEW_LITERAL(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
			)]);
			enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
#endif // RYTHE_DEBUG

		if (_applicationInfo.windowHandle != invalid_native_window_handle)
		{
			if (!surfaceExtensionActive)
			{
				enabledExtensionProperties.push_back(availableExtensions[get_extension_index(
					availableExtensions, MAKE_HASHED_STRING_VIEW_LITERAL(VK_KHR_SURFACE_EXTENSION_NAME)
				)]);
				enabledExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
			}

			if (!platformSurfaceExtensionActive)
			{
				enabledExtensionProperties.push_back(availableExtensions[get_extension_index(
					availableExtensions, MAKE_HASHED_STRING_VIEW_LITERAL(VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME)
				)]);
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

		const VkApplicationInfo applicationInfo{
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

		const VkInstanceCreateInfo instanceCreateInfo{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pApplicationInfo = &applicationInfo,
			.enabledLayerCount = static_cast<rsl::uint32>(enabledLayers.size()),
			.ppEnabledLayerNames = enabledLayers.data(),
			.enabledExtensionCount = static_cast<rsl::uint32>(enabledExtensions.size()),
			.ppEnabledExtensionNames = enabledExtensions.data(),
		};

		auto& impl = get_native_ref(*this);

		VkInstance vkInstance = VK_NULL_HANDLE;
		VkResult result = impl.vkCreateInstance(&instanceCreateInfo, &impl.allocCallbacks, &vkInstance);

		if (result != VK_SUCCESS || vkInstance == VK_NULL_HANDLE)
		{
			std::cout << "Failed to create Vulkan Instance\n";
			return {};
		}

		native_instance_vk* nativeInstance = allocate<native_instance_vk>(impl.alloc);
		nativeInstance->alloc = &impl.alloc;
		nativeInstance->allocCallbacks = &impl.allocCallbacks;
		nativeInstance->instance = vkInstance;
		nativeInstance->graphicsLib = *this;

		if (!nativeInstance->load_functions(enabledExtensions))
		{
			if (nativeInstance->vkDestroyInstance)
			{
				nativeInstance->vkDestroyInstance(nativeInstance->instance, &impl.allocCallbacks);
			}

			deallocate<native_instance_vk>(impl.alloc, nativeInstance);
			return {};
		}

		nativeInstance->applicationInfo = _applicationInfo;
		nativeInstance->apiVersion = apiVersion;
		nativeInstance->enabledLayers = std::move(enabledLayerProperties);
		nativeInstance->enabledExtensions = std::move(enabledExtensionProperties);

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

		impl->vkDestroyInstance(impl->instance, impl->allocCallbacks);

		m_nativeInstance = invalid_native_instance;
		deallocate<native_instance_vk>(*impl->alloc, impl);
	}

	std::span<physical_device> instance::create_physical_devices(bool forceRefresh)
	{
		auto& impl = get_native_ref(*this);

		if (forceRefresh || impl.physicalDevices.empty())
		{
			rsl::uint32 deviceCount = 0;
			VkResult result = impl.vkEnumeratePhysicalDevices(impl.instance, &deviceCount, nullptr);
			if (result != VK_SUCCESS || deviceCount == 0)
			{
				std::cout << "Could not query the number of physical devices.\n";
				return {};
			}

			std::vector<VkPhysicalDevice> physicalDevicesBuffer;
			physicalDevicesBuffer.resize(deviceCount);
			result = impl.vkEnumeratePhysicalDevices(impl.instance, &deviceCount, physicalDevicesBuffer.data());

			if (result != VK_SUCCESS || deviceCount == 0)
			{
				std::cout << "Could not enumerate physical devices.\n";
				return {};
			}

			release_physical_devices();
			impl.physicalDevices.reserve(deviceCount);
			for (auto& pd : physicalDevicesBuffer)
			{
				auto& physicalDevice = impl.physicalDevices.emplace_back();
				native_physical_device_vk* nativePhysicalDevice = allocate<native_physical_device_vk>(*impl.alloc);
				set_native_handle(physicalDevice, create_native_handle(nativePhysicalDevice));

				nativePhysicalDevice->alloc = impl.alloc;
				nativePhysicalDevice->allocCallbacks = impl.allocCallbacks;
				nativePhysicalDevice->physicalDevice = pd;
				nativePhysicalDevice->instance = *this;

#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION(name) nativePhysicalDevice->name = impl.name;
#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION_FROM_EXTENSION(name, extension)                                 \
	nativePhysicalDevice->name = impl.name;
#define INSTANCE_LEVEL_DEVICE_VULKAN_FUNCTION(name) nativePhysicalDevice->name = impl.name;
#include "impl/list_of_vulkan_functions.inl"
			}
		}

		return impl.physicalDevices;
	}

	void instance::release_physical_devices()
	{
		auto* impl = get_native_ptr(*this);

		if (!impl)
		{
			return;
		}

		for (auto& device : impl->physicalDevices) { device.release(); }

		impl->physicalDevices.clear();
	}

	const application_info& instance::get_application_info() const noexcept
	{
		return get_native_ref(*this).applicationInfo;
	}

	const semver::version& instance::get_api_version() const noexcept
	{
		return get_native_ref(*this).apiVersion;
	}

	namespace
	{
		[[nodiscard]] physical_device copy_physical_device(allocator& alloc, physical_device src)
		{
			physical_device copy;
			set_native_handle(
				copy, create_native_handle(allocate<native_physical_device_vk>(alloc, 1, get_native_ref(src)))
			);

			return copy;
		}

		[[nodiscard]] render_device create_render_device_no_extension_check(
			physical_device& physicalDevice, std::span<const queue_description> queueDesciptions,
			std::span<const rsl::cstring> extensions, std::span<const rsl::cstring> layers
		)
		{
			auto& impl = get_native_ref(physicalDevice);

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
			impl.vkGetPhysicalDeviceFeatures(impl.physicalDevice, &features);

			const VkDeviceCreateInfo deviceCreateInfo{
				.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.queueCreateInfoCount = static_cast<rsl::uint32>(queueCreateInfos.size()),
				.pQueueCreateInfos = queueCreateInfos.data(),
				.enabledLayerCount = static_cast<rsl::uint32>(layers.size()),
				.ppEnabledLayerNames = layers.data(),
				.enabledExtensionCount = static_cast<rsl::uint32>(extensions.size()),
				.ppEnabledExtensionNames = extensions.data(),
				.pEnabledFeatures = &features,
			};

			VkDevice device = VK_NULL_HANDLE;

			{
				VkResult result =
					impl.vkCreateDevice(impl.physicalDevice, &deviceCreateInfo, impl.allocCallbacks, &device);

				if (result != VK_SUCCESS || device == VK_NULL_HANDLE)
				{
					return {};
				}
			}

			auto* renderDevicePtr = allocate<native_render_device_vk>(*impl.alloc);
			renderDevicePtr->alloc = impl.alloc;
			renderDevicePtr->allocCallbacks = impl.allocCallbacks;
			renderDevicePtr->device = device;

#define INSTANCE_LEVEL_DEVICE_VULKAN_FUNCTION(name) renderDevicePtr->name = impl.name;
#include "impl/list_of_vulkan_functions.inl"

			if (!renderDevicePtr->load_functions(extensions))
			{
				if (renderDevicePtr->vkDestroyDevice)
				{
					renderDevicePtr->vkDestroyDevice(renderDevicePtr->device, impl.allocCallbacks);
				}

				deallocate<native_render_device_vk>(*impl.alloc, renderDevicePtr);
				return {};
			}

			renderDevicePtr->physicalDevice = copy_physical_device(*impl.alloc, physicalDevice);
			set_native_handle(impl.renderDevice, create_native_handle(renderDevicePtr));

			renderDevicePtr->queues.resize(queueDesciptions.size());
			for (rsl::size_type i = 0; i < queueCreateInfos.size(); i++)
			{
				auto& info = queueCreateInfos[i];
				auto& mapping = queueMapping[info.queueFamilyIndex];

				for (rsl::size_type queueIndex = 0; queueIndex < info.queueCount; queueIndex++)
				{
					VkQueue vkQueue = VK_NULL_HANDLE;
					renderDevicePtr->vkGetDeviceQueue(
						renderDevicePtr->device, info.queueFamilyIndex, static_cast<rsl::uint32>(queueIndex), &vkQueue
					);
					auto inputIndex = mapping.inputOrderIndex[queueIndex];

					if (vkQueue == VK_NULL_HANDLE)
					{
						std::cout << "Failed to create queue " << inputIndex << '\n';
						continue;
					}

					auto& queue = renderDevicePtr->queues[inputIndex];

					native_queue_vk* nativeQueue = allocate<native_queue_vk>(*impl.alloc);
					nativeQueue->alloc = impl.alloc;
					nativeQueue->allocCallbacks = impl.allocCallbacks;
					nativeQueue->queue = vkQueue;
					nativeQueue->renderDevice = impl.renderDevice;
					nativeQueue->queueIndex = inputIndex;
					nativeQueue->familyIndex = info.queueFamilyIndex;
					nativeQueue->family = queueFamilies[info.queueFamilyIndex];
					nativeQueue->priority = queueDesciptions[inputIndex].priority;

					set_native_handle(queue, create_native_handle(nativeQueue));
				}
			}

			return impl.renderDevice;
		}
	} // namespace

	[[nodiscard]] surface instance::create_surface()
	{
		auto& impl = get_native_ref(*this);

		if (impl.applicationInfo.windowHandle == invalid_native_window_handle)
		{
			return {};
		}

		VkSurfaceKHR vkSurface = VK_NULL_HANDLE;

#if RYTHE_PLATFORM_WINDOWS
		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0,
			.hinstance = get_hinstance(impl.applicationInfo.windowHandle),
			.hwnd = get_hwnd(impl.applicationInfo.windowHandle),
		};

		if (impl.vkCreateWin32SurfaceKHR(impl.instance, &surfaceCreateInfo, impl.allocCallbacks, &vkSurface) !=
			VK_SUCCESS)
		{
			return {};
		}
#elif RYTHE_PLATFORM_LINUX
	#ifdef RYTHE_SURFACE_XCB
		VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0,
			.connection = get_connection(impl.applicationInfo.windowHandle),
			.window = get_window(impl.applicationInfo.windowHandle),
		};

		if (impl.vkCreateXcbSurfaceKHR(impl.instance, &surfaceCreateInfo, impl.allocCallbacks, &vkSurface) !=
			VK_SUCCESS)
		{
			return {};
		}

	#elif RYTHE_SURFACE_XLIB
		VkXlibSurfaceCreateInfoKHR surfaceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0,
			.dpy = get_display(impl.applicationInfo.windowHandle),
			.window = get_window(impl.applicationInfo.windowHandle),
		};

		if (impl.vkCreateXlibSurfaceKHR(impl.instance, &surfaceCreateInfo, impl.allocCallbacks, &vkSurface) !=
			VK_SUCCESS)
		{
			return {};
		}
	#endif
#endif

		native_surface_vk* nativeSurface = allocate<native_surface_vk>(*impl.alloc);
		nativeSurface->surface = vkSurface;
		nativeSurface->alloc = impl.alloc;
		nativeSurface->allocCallbacks = impl.allocCallbacks;
		nativeSurface->instance = *this;

		surface result;
		set_native_handle(result, create_native_handle(nativeSurface));

		return result;
	}

	[[nodiscard]] render_device instance::auto_select_and_create_device(
		const physical_device_description& physicalDeviceDescription,
		std::span<const queue_description> queueDesciptions, surface surface,
		std::span<const rsl::hashed_string> extensions
	)
	{
		using namespace rsl::hashed_string_literals;
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

		bool presentingApplication = get_application_info().windowHandle != invalid_native_window_handle;

		std::vector<rsl::cstring> enabledExtensions;
		std::vector<rsl::id_type> enabledExtensionHashes;
		enabledExtensions.reserve(extensions.size() + (presentingApplication ? 1 : 0));
		enabledExtensionHashes.reserve(enabledExtensions.size());

		bool swapchainExtensionPresent = false;

		constexpr auto swapchainExtensionName = MAKE_HASHED_STRING_VIEW_LITERAL(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		for (auto& extensionName : extensions)
		{
			enabledExtensions.push_back(extensionName.c_str());
			enabledExtensionHashes.push_back(extensionName.value);

			if (presentingApplication && extensionName == swapchainExtensionName)
			{
				swapchainExtensionPresent = true;
			}
		}

		if (presentingApplication && !swapchainExtensionPresent)
		{
			enabledExtensions.push_back(swapchainExtensionName.data());
			enabledExtensionHashes.push_back(swapchainExtensionName.value);
		}

		if (!presentingApplication && swapchainExtensionPresent)
		{
			std::cout << "Swapchain extension is activated, but no window handle was provided.\n";
			return {};
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

			for (rsl::size_type i = 0; i < enabledExtensions.size(); i++)
			{
				rsl::hashed_string_view hashStrView;
				hashStrView.str = enabledExtensions[i];
				hashStrView.value = enabledExtensionHashes[i];

				if (!device.is_extension_available(hashStrView))
				{
					continue;
				}
			}

			std::vector<queue_family_selection> queueFamilySelections;
			queueFamilySelections.resize(queueDesciptions.size());
			if (!device.get_queue_family_selection(queueFamilySelections, queueDesciptions, surface))
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
			return {};
		}

		auto& impl = get_native_ref(*this);
		std::vector<rsl::cstring> enabledLayers;
		enabledLayers.reserve(impl.enabledLayers.size());
		for (auto& layer : impl.enabledLayers) { enabledLayers.push_back(layer.name.c_str()); }

		auto result = create_render_device_no_extension_check(
			physicalDevices[selectedDevice], queueDesciptions, enabledExtensions, enabledLayers
		);

		release_physical_devices();

		return result;
	}

	bool native_instance_vk::load_functions(std::span<const rsl::cstring> extensions)
	{
		auto& lib = get_native_ref(graphicsLib);

#define INSTANCE_LEVEL_VULKAN_FUNCTION(name)                                                                           \
	name = std::bit_cast<PFN_##name>(lib.vkGetInstanceProcAddr(instance, #name));                                      \
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
			name = std::bit_cast<PFN_##name>(lib.vkGetInstanceProcAddr(instance, #name));                              \
			if (!name)                                                                                                 \
			{                                                                                                          \
				std::cout << "Could not load instance-level Vulkan function \"" #name "\"\n";                          \
				return false;                                                                                          \
			}                                                                                                          \
			break;                                                                                                     \
		}                                                                                                              \
	}

#define INSTANCE_LEVEL_PHYSICAL_DEVICE_VULKAN_FUNCTION(name)                                                           \
	name = std::bit_cast<PFN_##name>(lib.vkGetInstanceProcAddr(instance, #name));                                      \
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
			name = std::bit_cast<PFN_##name>(lib.vkGetInstanceProcAddr(instance, #name));                              \
			if (!name)                                                                                                 \
			{                                                                                                          \
				std::cout << "Could not load instance-level Vulkan function \"" #name "\"\n";                          \
				return false;                                                                                          \
			}                                                                                                          \
			break;                                                                                                     \
		}                                                                                                              \
	}

#define INSTANCE_LEVEL_DEVICE_VULKAN_FUNCTION(name)                                                                    \
	name = std::bit_cast<PFN_##name>(lib.vkGetInstanceProcAddr(instance, #name));                                      \
	if (!name)                                                                                                         \
	{                                                                                                                  \
		std::cout << "Could not load instance-level Vulkan function \"" #name "\"\n";                                  \
		return false;                                                                                                  \
	}

#include "impl/list_of_vulkan_functions.inl"

		return true;
	}

	surface::operator bool() const noexcept
	{
		auto ptr = get_native_ptr(*this);
		return ptr != nullptr && ptr->surface != VK_NULL_HANDLE;
	}

	void surface::release()
	{
		auto impl = get_native_ptr(*this);
		if (!impl)
		{
			return;
		}

		auto nativeInstance = get_native_ptr(impl->instance);
		nativeInstance->vkDestroySurfaceKHR(nativeInstance->instance, impl->surface, impl->allocCallbacks);

		m_nativeSurface = invalid_native_surface;
		deallocate<native_surface_vk>(*impl->alloc, impl);
	}

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
		deallocate<native_physical_device_vk>(*impl->alloc, impl);
	}

	namespace
	{
		surface_transform_flags map_vk_surface_transform_flags(VkSurfaceTransformFlagBitsKHR flags)
		{
			surface_transform_flags result = rsl::enum_flags::make_zero<surface_transform_flags>();

			result = rsl::enum_flags::set_flag(
				result, surface_transform_flags::identity,
				rsl::enum_flags::has_flag(flags, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
			);
			result = rsl::enum_flags::set_flag(
				result, surface_transform_flags::rotate90,
				rsl::enum_flags::has_flag(flags, VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR)
			);
			result = rsl::enum_flags::set_flag(
				result, surface_transform_flags::rotate180,
				rsl::enum_flags::has_flag(flags, VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR)
			);
			result = rsl::enum_flags::set_flag(
				result, surface_transform_flags::rotate270,
				rsl::enum_flags::has_flag(flags, VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR)
			);
			result = rsl::enum_flags::set_flag(
				result, surface_transform_flags::horizontalMirror,
				rsl::enum_flags::has_flag(flags, VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR)
			);
			result = rsl::enum_flags::set_flag(
				result, surface_transform_flags::horizontalMirrorRotate90,
				rsl::enum_flags::has_flag(flags, VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR)
			);
			result = rsl::enum_flags::set_flag(
				result, surface_transform_flags::horizontalMirrorRotate180,
				rsl::enum_flags::has_flag(flags, VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR)
			);
			result = rsl::enum_flags::set_flag(
				result, surface_transform_flags::horizontalMirrorRotate270,
				rsl::enum_flags::has_flag(flags, VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR)
			);
			result = rsl::enum_flags::set_flag(
				result, surface_transform_flags::inherit,
				rsl::enum_flags::has_flag(flags, VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR)
			);

			return result;
		}

		composite_alpha_flags map_vk_composite_alpha_flags(VkCompositeAlphaFlagBitsKHR flags)
		{
			composite_alpha_flags result = rsl::enum_flags::make_zero<composite_alpha_flags>();

			result = rsl::enum_flags::set_flag(
				result, composite_alpha_flags::opaque,
				rsl::enum_flags::has_flag(flags, VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
			);
			result = rsl::enum_flags::set_flag(
				result, composite_alpha_flags::preMultiplied,
				rsl::enum_flags::has_flag(flags, VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
			);
			result = rsl::enum_flags::set_flag(
				result, composite_alpha_flags::postMultiplied,
				rsl::enum_flags::has_flag(flags, VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
			);
			result = rsl::enum_flags::set_flag(
				result, composite_alpha_flags::inherit,
				rsl::enum_flags::has_flag(flags, VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
			);

			return result;
		}

		image_usage_flags map_vk_image_usage_flags(VkImageUsageFlagBits flags)
		{
			image_usage_flags result = rsl::enum_flags::make_zero<image_usage_flags>();

			result = rsl::enum_flags::set_flag(
				result, image_usage_flags::transferSrc,
				rsl::enum_flags::has_flag(flags, VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
			);
			result = rsl::enum_flags::set_flag(
				result, image_usage_flags::transferDst,
				rsl::enum_flags::has_flag(flags, VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			);
			result = rsl::enum_flags::set_flag(
				result, image_usage_flags::sampled, rsl::enum_flags::has_flag(flags, VK_IMAGE_USAGE_SAMPLED_BIT)
			);
			result = rsl::enum_flags::set_flag(
				result, image_usage_flags::storage, rsl::enum_flags::has_flag(flags, VK_IMAGE_USAGE_STORAGE_BIT)
			);
			result = rsl::enum_flags::set_flag(
				result, image_usage_flags::colorAttachment,
				rsl::enum_flags::has_flag(flags, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
			);
			result = rsl::enum_flags::set_flag(
				result, image_usage_flags::depthStencilAttachment,
				rsl::enum_flags::has_flag(flags, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
			);
			result = rsl::enum_flags::set_flag(
				result, image_usage_flags::transientAttachment,
				rsl::enum_flags::has_flag(flags, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
			);
			result = rsl::enum_flags::set_flag(
				result, image_usage_flags::inputAttachment,
				rsl::enum_flags::has_flag(flags, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
			);
			result = rsl::enum_flags::set_flag(
				result, image_usage_flags::fragmentShadingRateAttachment,
				rsl::enum_flags::has_flag(flags, VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR)
			);
			result = rsl::enum_flags::set_flag(
				result, image_usage_flags::fragmentDensityMap,
				rsl::enum_flags::has_flag(flags, VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT)
			);
			result = rsl::enum_flags::set_flag(
				result, image_usage_flags::videoDecodeDst,
				rsl::enum_flags::has_flag(flags, VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR)
			);
			result = rsl::enum_flags::set_flag(
				result, image_usage_flags::videoDecodeSrc,
				rsl::enum_flags::has_flag(flags, VK_IMAGE_USAGE_VIDEO_DECODE_SRC_BIT_KHR)
			);
			result = rsl::enum_flags::set_flag(
				result, image_usage_flags::videoDecodeDpb,
				rsl::enum_flags::has_flag(flags, VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR)
			);
			result = rsl::enum_flags::set_flag(
				result, image_usage_flags::videoEncodeDst,
				rsl::enum_flags::has_flag(flags, VK_IMAGE_USAGE_VIDEO_ENCODE_DST_BIT_KHR)
			);
			result = rsl::enum_flags::set_flag(
				result, image_usage_flags::videoEncodeSrc,
				rsl::enum_flags::has_flag(flags, VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR)
			);
			result = rsl::enum_flags::set_flag(
				result, image_usage_flags::videoEncodeDpb,
				rsl::enum_flags::has_flag(flags, VK_IMAGE_USAGE_VIDEO_ENCODE_DPB_BIT_KHR)
			);
			result = rsl::enum_flags::set_flag(
				result, image_usage_flags::invocationMaskHUAWEI,
				rsl::enum_flags::has_flag(flags, VK_IMAGE_USAGE_INVOCATION_MASK_BIT_HUAWEI)
			);
			result = rsl::enum_flags::set_flag(
				result, image_usage_flags::attachmentFeedbackLoop,
				rsl::enum_flags::has_flag(flags, VK_IMAGE_USAGE_ATTACHMENT_FEEDBACK_LOOP_BIT_EXT)
			);
			result = rsl::enum_flags::set_flag(
				result, image_usage_flags::sampleWeightQCOM,
				rsl::enum_flags::has_flag(flags, VK_IMAGE_USAGE_SAMPLE_WEIGHT_BIT_QCOM)
			);
			result = rsl::enum_flags::set_flag(
				result, image_usage_flags::sampleBlockMatchQCOM,
				rsl::enum_flags::has_flag(flags, VK_IMAGE_USAGE_SAMPLE_BLOCK_MATCH_BIT_QCOM)
			);
			result = rsl::enum_flags::set_flag(
				result, image_usage_flags::hostTransfer,
				rsl::enum_flags::has_flag(flags, VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT)
			);

			return result;
		}
	} // namespace

	const surface_capabilities& physical_device::get_surface_capabilities(surface _surface, bool forceRefresh)
	{
		auto& impl = get_native_ref(*this);

		if (forceRefresh || !impl.surfaceCapsLoaded)
		{
			auto& nativeSurface = get_native_ref(_surface);

			VkSurfaceCapabilitiesKHR vkSurfaceCaps;
			impl.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(impl.physicalDevice, nativeSurface.surface, &vkSurfaceCaps);

			impl.surfaceCaps.minImageCount = static_cast<rsl::size_type>(vkSurfaceCaps.minImageCount);
			impl.surfaceCaps.maxImageCount = static_cast<rsl::size_type>(vkSurfaceCaps.maxImageCount);
			impl.surfaceCaps.currentExtent =
				rsl::math::uint2(vkSurfaceCaps.currentExtent.width, vkSurfaceCaps.currentExtent.height);
			impl.surfaceCaps.minImageExtent =
				rsl::math::uint2(vkSurfaceCaps.minImageExtent.width, vkSurfaceCaps.minImageExtent.height);
			impl.surfaceCaps.maxImageExtent =
				rsl::math::uint2(vkSurfaceCaps.maxImageExtent.width, vkSurfaceCaps.maxImageExtent.height);
			impl.surfaceCaps.maxImageArrayLayers = static_cast<rsl::size_type>(vkSurfaceCaps.maxImageArrayLayers);
			impl.surfaceCaps.supportedTransforms = map_vk_surface_transform_flags(
				static_cast<VkSurfaceTransformFlagBitsKHR>(vkSurfaceCaps.supportedTransforms)
			);
			impl.surfaceCaps.currentTransform = map_vk_surface_transform_flags(vkSurfaceCaps.currentTransform);
			impl.surfaceCaps.supportedCompositeAlpha = map_vk_composite_alpha_flags(
				static_cast<VkCompositeAlphaFlagBitsKHR>(vkSurfaceCaps.supportedCompositeAlpha)
			);
			impl.surfaceCaps.supportedUsageFlags =
				map_vk_image_usage_flags(static_cast<VkImageUsageFlagBits>(vkSurfaceCaps.supportedUsageFlags));

			impl.surfaceCapsLoaded = true;
		}

		return impl.surfaceCaps;
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

	const physical_device_properties& physical_device::get_properties(bool forceRefresh)
	{
		auto& impl = get_native_ref(*this);

		if (forceRefresh || !impl.propertiesLoaded)
		{
			VkPhysicalDeviceProperties props;
			impl.vkGetPhysicalDeviceProperties(impl.physicalDevice, &props);

			impl.properties.apiVersion = decomposeVkVersion(props.apiVersion);
			impl.properties.driverVersion = decomposeVkVersion(props.driverVersion);
			impl.properties.vendorID = props.vendorID;
			impl.properties.deviceID = props.deviceID;
			impl.properties.deviceType = map_vk_physical_device_type(props.deviceType);
			impl.properties.deviceName = props.deviceName;
			map_vk_physical_device_limits(impl.properties.limits, props.limits);
			map_vk_physical_device_sparse_properties(impl.properties.sparseProperties, props.sparseProperties);

			impl.propertiesLoaded = true;
		}

		return impl.properties;
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
		auto& impl = get_native_ref(*this);

		if (forceRefresh || !impl.featuresLoaded)
		{
			VkPhysicalDeviceFeatures features;
			impl.vkGetPhysicalDeviceFeatures(impl.physicalDevice, &features);
			map_vk_physical_device_features(impl.features, features);

			impl.featuresLoaded = true;
		}

		return impl.features;
	}

	std::span<const extension_properties> physical_device::get_available_extensions(bool forceRefresh)
	{
		auto& impl = get_native_ref(*this);

		if (forceRefresh || impl.availableExtensions.empty())
		{
			rsl::uint32 extensionCount = 0;

			VkResult result =
				impl.vkEnumerateDeviceExtensionProperties(impl.physicalDevice, nullptr, &extensionCount, nullptr);
			if (result != VK_SUCCESS || extensionCount == 0)
			{
				std::cout << "Count not query the number of device extensions.\n";
				return {};
			}

			std::vector<VkExtensionProperties> extensionPropertiesBuffer;
			extensionPropertiesBuffer.resize(extensionCount);
			result = impl.vkEnumerateDeviceExtensionProperties(
				impl.physicalDevice, nullptr, &extensionCount, extensionPropertiesBuffer.data()
			);

			if (result != VK_SUCCESS || extensionCount == 0)
			{
				std::cout << "Could not enumerate device extensions.\n";
				return {};
			}

			impl.availableExtensions.clear();
			impl.availableExtensions.reserve(extensionCount);
			for (auto& extension : extensionPropertiesBuffer)
			{
				impl.availableExtensions.push_back(extension_properties{
					.name = rsl::hashed_string(extension.extensionName),
					.specVersion = decomposeVkVersion(extension.specVersion),
				});
			}
		}

		return impl.availableExtensions;
	}

	bool physical_device::is_extension_available(rsl::hashed_string_view extensionName)
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

	std::span<const queue_family_properties>
	physical_device::get_available_queue_families(surface surface, bool forceRefresh)
	{
		auto& impl = get_native_ref(*this);

		if (forceRefresh || impl.availableQueueFamilies.empty())
		{
			rsl::uint32 queueFamilyCount = 0;

			impl.vkGetPhysicalDeviceQueueFamilyProperties(impl.physicalDevice, &queueFamilyCount, nullptr);
			if (queueFamilyCount == 0)
			{
				std::cout << "Count not query the number of queue families.\n";
				return {};
			}

			std::vector<VkQueueFamilyProperties> queueFamiliesBuffer;
			queueFamiliesBuffer.resize(queueFamilyCount);
			impl.vkGetPhysicalDeviceQueueFamilyProperties(
				impl.physicalDevice, &queueFamilyCount, queueFamiliesBuffer.data()
			);

			if (queueFamilyCount == 0)
			{
				std::cout << "Could not get queue family properties.\n";
				return {};
			}

			impl.availableQueueFamilies.clear();
			impl.availableQueueFamilies.reserve(queueFamilyCount);

			auto& nativeInstance = get_native_ref(impl.instance);

			bool surfaceCreated = false;
			if (!surface)
			{
				surface = impl.instance.create_surface();
				surfaceCreated = true;
			}

			auto* nativeSurface = get_native_ptr(surface);

			for (rsl::uint32 queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++)
			{
				auto& queueFamily = queueFamiliesBuffer[queueFamilyIndex];

				VkBool32 supportsPresent = VK_FALSE;

				if (nativeSurface)
				{
					if (nativeInstance.vkGetPhysicalDeviceSurfaceSupportKHR(
							impl.physicalDevice, queueFamilyIndex, nativeSurface->surface, &supportsPresent
						) != VK_SUCCESS)
					{
						supportsPresent = VK_FALSE;
					}
				}

				impl.availableQueueFamilies.push_back({
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

			if (surfaceCreated)
			{
				surface.release();
			}
		}

		return impl.availableQueueFamilies;
	}

	bool physical_device::get_queue_family_selection(
		std::span<queue_family_selection> queueFamilySelections, std::span<const queue_description> queueDesciptions,
		surface surface
	)
	{
		for (rsl::size_type i = 0; i < queueDesciptions.size(); i++)
		{
			queueFamilySelections[i].familyIndex = queueDesciptions[i].queueFamilyIndexOverride;
		}

		auto queueFamilies = get_available_queue_families(surface);

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

	bool physical_device::in_use() const noexcept
	{
		return get_native_ref(*this).renderDevice;
	}

	[[nodiscard]] render_device physical_device::create_render_device(
		std::span<const queue_description> queueDesciptions, std::span<const rsl::hashed_string> extensions
	)
	{
		using namespace rsl::hashed_string_literals;

		auto& impl = get_native_ref(*this);

		bool presentingApplication = impl.instance.get_application_info().windowHandle != invalid_native_window_handle;

		std::vector<rsl::cstring> enabledExtensions;
		enabledExtensions.reserve(extensions.size() + (presentingApplication ? 1 : 0));

		bool swapchainExtensionPresent = false;

		for (auto& extensionName : extensions)
		{
			if (is_extension_available(extensionName))
			{
				enabledExtensions.push_back(extensionName.c_str());
			}
			else
			{
				std::cout << "Extension \"" << extensionName << "\" is not available.\n";
			}

			if (presentingApplication &&
				extensionName == MAKE_HASHED_STRING_VIEW_LITERAL(VK_KHR_SWAPCHAIN_EXTENSION_NAME))
			{
				swapchainExtensionPresent = true;
			}
		}

		if (presentingApplication && !swapchainExtensionPresent)
		{
			enabledExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}

		if (!presentingApplication && swapchainExtensionPresent)
		{
			std::cout << "Swapchain extension is activated, but no window handle was provided.\n";
			return {};
		}

		auto& instancePtr = get_native_ref(impl.instance);
		std::vector<rsl::cstring> enabledLayers;
		enabledLayers.reserve(instancePtr.enabledLayers.size());
		for (auto& layer : instancePtr.enabledLayers) { enabledLayers.push_back(layer.name.c_str()); }

		return create_render_device_no_extension_check(*this, queueDesciptions, enabledExtensions, enabledLayers);
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

		impl->vkDestroyDevice(impl->device, impl->allocCallbacks);

		impl->physicalDevice.release();

		m_nativeRenderDevice = invalid_native_render_device;
		deallocate<native_render_device_vk>(*impl->alloc, impl);
	}

	std::span<queue> render_device::get_queues() noexcept
	{
		return get_native_ref(*this).queues;
	}

	physical_device render_device::get_physical_device() const noexcept
	{
		return get_native_ref(*this).physicalDevice;
	}

	bool native_render_device_vk::load_functions(std::span<const rsl::cstring> extensions)
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

	queue::operator bool() const noexcept
	{
		auto* impl = get_native_ptr(*this);
		return impl != nullptr && impl->queue != VK_NULL_HANDLE;
	}

	void queue::release()
	{
		auto* impl = get_native_ptr(*this);
		if (!impl)
		{
			return;
		}

		m_nativeQueue = invalid_native_queue;
		deallocate<native_queue_vk>(*impl->alloc, impl);
	}

	rsl::size_type queue::get_index() const noexcept
	{
		return get_native_ref(*this).queueIndex;
	}

	rsl::size_type queue::get_family_index() const noexcept
	{
		return get_native_ref(*this).familyIndex;
	}

	queue_priority queue::get_priority() const noexcept
	{
		return get_native_ref(*this).priority;
	}

	const queue_family_properties& queue::get_family() const noexcept
	{
		return get_native_ref(*this).family;
	}

	namespace
	{
		bool create_command_pool(queue q, command_pool& commandPool, VkCommandPoolCreateFlags flags)
		{
			auto& impl = get_native_ref(q);
			auto& renderDevice = get_native_ref(impl.renderDevice);
			const VkCommandPoolCreateInfo commandPoolCreateInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.pNext = nullptr,
				.flags = flags,
				.queueFamilyIndex = static_cast<rsl::uint32>(impl.familyIndex),
			};

			VkCommandPool vkCommandPool = VK_NULL_HANDLE;
			VkResult result = renderDevice.vkCreateCommandPool(
				renderDevice.device, &commandPoolCreateInfo, impl.allocCallbacks, &vkCommandPool
			);

			if (result != VK_SUCCESS || vkCommandPool == VK_NULL_HANDLE)
			{
				std::cout << "Failed to create command pool for queue " << impl.queueIndex << '\n';
				return false;
			}

			native_command_pool_vk* nativeCommandPool = allocate<native_command_pool_vk>(*impl.alloc);

			nativeCommandPool->alloc = impl.alloc;
			nativeCommandPool->allocCallbacks = impl.allocCallbacks;
			nativeCommandPool->commandPool = vkCommandPool;
			nativeCommandPool->renderDevice = impl.renderDevice;

			set_native_handle(commandPool, create_native_handle(nativeCommandPool));
			return true;
		}
	} // namespace

	[[nodiscard]] persistent_command_pool queue::create_persistent_command_pool(bool protectedCommandBuffers)
	{
		persistent_command_pool commandPool;

		if (!create_command_pool(
				*this, commandPool,
				VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
					(protectedCommandBuffers ? VK_COMMAND_POOL_CREATE_PROTECTED_BIT : 0)
			))
		{
			return {};
		}

		return commandPool;
	}

	transient_command_pool queue::create_transient_command_pool(bool protectedCommandBuffers)
	{
		transient_command_pool commandPool;

		if (!create_command_pool(
				*this, commandPool,
				VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
					(protectedCommandBuffers ? VK_COMMAND_POOL_CREATE_PROTECTED_BIT : 0)
			))
		{
			return {};
		}

		return commandPool;
	}

	namespace
	{
		bool create_command_buffers(
			native_command_pool_vk& impl, std::span<command_buffer> buffers,
			std::span<VkCommandBuffer> commandBuffersBuffer, command_buffer_level level
		)
		{
			rsl_assert_consistent(buffers.size() <= commandBuffersBuffer.size());

			auto& renderDevice = get_native_ref(impl.renderDevice);

			const VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.pNext = nullptr,
				.commandPool = impl.commandPool,
				.level =
					(level == command_buffer_level::primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY
															: VK_COMMAND_BUFFER_LEVEL_SECONDARY),
				.commandBufferCount = static_cast<rsl::uint32>(buffers.size()),
			};

			VkResult result = renderDevice.vkAllocateCommandBuffers(
				renderDevice.device, &commandBufferAllocateInfo, commandBuffersBuffer.data()
			);

			if (result != VK_SUCCESS)
			{
				return false;
			}

			for (rsl::size_type i = 0; i < buffers.size(); i++)
			{
				command_buffer& buffer = buffers[i];
				auto* nativeCommandBuffer = get_native_ptr(buffer);
				if (!nativeCommandBuffer)
				{
					nativeCommandBuffer = allocate<native_command_buffer_vk>(*impl.alloc);
					set_native_handle(buffer, create_native_handle(nativeCommandBuffer));
				}

				nativeCommandBuffer->device = impl.renderDevice;
				nativeCommandBuffer->nextUnusedIndex = i + 1;
				nativeCommandBuffer->indexInPool = i;
				nativeCommandBuffer->level = level;
				nativeCommandBuffer->commandBuffer = commandBuffersBuffer[i];
			}

			return true;
		}
	} // namespace

	command_pool::operator bool() const noexcept
	{
		auto* impl = get_native_ptr(*this);
		return impl != nullptr && impl->commandPool != VK_NULL_HANDLE;
	}

	void persistent_command_pool::release()
	{
		auto* impl = get_native_ptr(*this);
		if (!impl)
		{
			return;
		}

		for (auto& commandBuffer : impl->primaryCommandBuffers.commandBuffers)
		{
			auto* ptr = get_native_ptr(commandBuffer);
			if (ptr)
			{
				deallocate<native_command_buffer_vk>(*impl->alloc, ptr);
			}
		}

		for (auto& commandBuffer : impl->secondaryCommandBuffers.commandBuffers)
		{
			auto* ptr = get_native_ptr(commandBuffer);
			if (ptr)
			{
				deallocate<native_command_buffer_vk>(*impl->alloc, ptr);
			}
		}

		auto& renderDevice = get_native_ref(impl->renderDevice);

		renderDevice.vkDestroyCommandPool(renderDevice.device, impl->commandPool, impl->allocCallbacks);

		m_nativeCommandPool = invalid_native_command_pool;
		deallocate<native_command_pool_vk>(*impl->alloc, impl);
	}

	rsl::size_type persistent_command_pool::get_capacity(command_buffer_level level) const noexcept
	{
		auto& impl = get_native_ref(*this);
		return level == command_buffer_level::primary ? impl.primaryCommandBuffers.commandBuffers.size()
													  : impl.secondaryCommandBuffers.commandBuffers.size();
	}

	rsl::size_type persistent_command_pool::get_unused_count(command_buffer_level level) const noexcept
	{
		auto& impl = get_native_ref(*this);
		return level == command_buffer_level::primary ? impl.primaryCommandBuffers.unusedCount
													  : impl.secondaryCommandBuffers.unusedCount;
	}

	void persistent_command_pool::reserve(rsl::size_type count, command_buffer_level level)
	{
		auto& impl = get_native_ref(*this);

		auto& commandBufferPool =
			level == command_buffer_level::primary ? impl.primaryCommandBuffers : impl.secondaryCommandBuffers;

		auto oldCount = commandBufferPool.commandBuffers.size();

		if (oldCount >= count)
		{
			return;
		}

		rsl::size_type additionalCount = count - oldCount;
		commandBufferPool.commandBuffers.resize(count);
		if (additionalCount > impl.commandBuffersBuffer.size())
		{
			impl.commandBuffersBuffer.resize(additionalCount);
		}

        [[maybe_unused]]bool result = create_command_buffers(
			impl, std::span(commandBufferPool.commandBuffers.data() + oldCount, additionalCount),
			impl.commandBuffersBuffer, level
		);
		rsl_soft_assert_msg_consistent(result, "failed to create command buffer");

		commandBufferPool.unusedCount += additionalCount;
	}

	command_buffer persistent_command_pool::get_command_buffer(command_buffer_level level)
	{
		auto& impl = get_native_ref(*this);

		auto& commandBufferPool =
			level == command_buffer_level::primary ? impl.primaryCommandBuffers : impl.secondaryCommandBuffers;

		if (commandBufferPool.unusedCount == 0)
		{
			reserve(commandBufferPool.lastUnusedIndex + 1, level);
		}
		else
		{
			rsl_assert_consistent(commandBufferPool.lastUnusedIndex < commandBufferPool.commandBuffers.size());
		}

		command_buffer commandBuffer = commandBufferPool.commandBuffers[commandBufferPool.lastUnusedIndex];

		auto& nativeCommandBuffer = get_native_ref(commandBuffer);
		nativeCommandBuffer.commandPoolStorage.set(*this);
		commandBufferPool.lastUnusedIndex = nativeCommandBuffer.nextUnusedIndex;
		commandBufferPool.unusedCount--;

		return commandBuffer;
	}

	void persistent_command_pool::return_command_buffer(command_buffer& commandBuffer)
	{
		auto& impl = get_native_ref(*this);
		auto& nativeCommandBuffer = get_native_ref(commandBuffer);


		auto& commandBufferPool = nativeCommandBuffer.level == command_buffer_level::primary
									  ? impl.primaryCommandBuffers
									  : impl.secondaryCommandBuffers;

		nativeCommandBuffer.nextUnusedIndex = commandBufferPool.lastUnusedIndex;
		commandBufferPool.lastUnusedIndex = nativeCommandBuffer.indexInPool;
		commandBufferPool.unusedCount++;

		set_native_handle(commandBuffer, invalid_native_command_buffer);
	}

	void transient_command_pool::release()
	{
		auto* impl = get_native_ptr(*this);
		if (!impl)
		{
			return;
		}

		auto& renderDevice = get_native_ref(impl->renderDevice);

		renderDevice.vkDestroyCommandPool(renderDevice.device, impl->commandPool, impl->allocCallbacks);

		m_nativeCommandPool = invalid_native_command_pool;
		deallocate<native_command_pool_vk>(*impl->alloc, impl);
	}

	rsl::size_type transient_command_pool::get_capacity([[maybe_unused]] command_buffer_level level) const noexcept
	{
		return rsl::size_type();
	}

	rsl::size_type transient_command_pool::get_unused_count([[maybe_unused]] command_buffer_level level) const noexcept
	{
		return rsl::size_type();
	}

	void
	transient_command_pool::reserve([[maybe_unused]] rsl::size_type count, [[maybe_unused]] command_buffer_level level)
	{
	}

	command_buffer transient_command_pool::get_command_buffer([[maybe_unused]] command_buffer_level level)
	{
		return command_buffer();
	}

	void transient_command_pool::return_command_buffer([[maybe_unused]] command_buffer& commandBuffer) {}

	command_buffer::operator bool() const noexcept
	{
		auto* impl = get_native_ptr(*this);
		return impl && impl->commandBuffer != VK_NULL_HANDLE;
	}

	void command_buffer::return_to_pool()
	{
		auto* impl = get_native_ptr(*this);
		if (impl)
		{
			impl->get_pool().return_command_buffer(*this);
		}
	}

} // namespace vk
