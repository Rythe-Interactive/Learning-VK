#pragma once

#include <rsl/hashed_string>
#include <rsl/math>
#include <rsl/platform>
#include <rsl/primitives>

#include <semver/semver.hpp>
#include <span>

#if RYTHE_PLATFORM_WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#define VC_EXTRALEAN
	#define NOMINMAX
	#include <windef.h>
	#include <minwinbase.h>
#endif

namespace vk
{
    struct allocator
	{
		void* (*allocFunc)(rsl::size_type size, void* userData) = nullptr;
		void* (*allocAlignedFunc)(rsl::size_type size, rsl::size_type allignment, void* userData) = nullptr;
		void* (*reallocFunc)(
			void* ptr, rsl::size_type oldSize, rsl::size_type newSize, rsl::size_type alignment, void* userData
		) = nullptr;
		void (*freeFunc)(void* ptr, rsl::size_type size, void* userData) = nullptr;
		void* userData = nullptr;
	};

#define DECLARE_API_TYPE(type)                                                                                         \
	DECLARE_OPAQUE_HANDLE(native_##type);                                                                              \
	class type;                                                                                                        \
	[[maybe_unused]] [[rythe_always_inline]] static void set_native_handle(type&, native_##type);


	DECLARE_API_TYPE(graphics_library);
	DECLARE_API_TYPE(surface)
	DECLARE_API_TYPE(instance)
	DECLARE_API_TYPE(physical_device)
	DECLARE_API_TYPE(render_device)
	DECLARE_API_TYPE(queue)
	DECLARE_API_TYPE(command_pool)
	DECLARE_API_TYPE(command_buffer)

#undef DECLARE_API_TYPE

	DECLARE_OPAQUE_HANDLE(native_window_handle);

#if RYTHE_PLATFORM_WINDOWS
	struct native_window_info_win32
	{
		allocator alloc;
		HINSTANCE hinstance;
		HWND hwnd;
	};

	[[nodiscard]] native_window_handle create_window_handle_win32(const native_window_info_win32& windowInfo);
#elif RYTHE_PLATFORM_LINUX
	#ifdef RYTHE_SURFACE_XCB
	struct native_window_info_xcb
	{
		allocator alloc;
		xcb_connection_t* connection;
		xcb_window_t window;
	};

	[[nodiscard]] native_window_handle create_window_handle_xcb(const native_window_info_xcb& windowInfo);
	#elif RYTHE_SURFACE_XLIB
	struct native_window_info_xlib
	{
		allocator alloc;
		Display* display;
		Window window;
	};

	[[nodiscard]] native_window_handle create_window_handle_xlib(const native_window_info_xlib& windowInfo);
	#endif
#endif

	void release_window_handle(native_window_handle handle);

	class graphics_library;

	[[nodiscard]] graphics_library init(allocator alloc = {});

	struct extension_properties
	{
		rsl::hashed_string name;
		semver::version specVersion;
	};

	struct application_info
	{
		std::string name;
		semver::version version;
		native_window_handle windowHandle;
	};

	struct layer_properties
	{
		rsl::hashed_string name;
		semver::version specVersion;
		semver::version implementationVersion;
		std::string description;
	};

	class instance;

	class graphics_library
	{
	public:
		operator bool() const noexcept;

		void release();

		std::span<const layer_properties> get_available_instance_layers(bool forceRefresh = false);
		bool is_instance_layer_available(rsl::hashed_string_view layerName);

		std::span<const extension_properties> get_available_instance_extensions(bool forceRefresh = false);
		bool is_instance_extension_available(rsl::hashed_string_view extensionName);

		[[nodiscard]] instance create_instance(
			const application_info& applicationInfo, const semver::version& apiVersion = {1, 0, 0},
			std::span<const rsl::hashed_string> layers = {}, std::span<const rsl::hashed_string> extensions = {}
		);

		[[rythe_always_inline]] native_graphics_library get_native_handle() const noexcept { return m_nativeGL; }

	private:
		native_graphics_library m_nativeGL;

		friend void set_native_handle(graphics_library&, native_graphics_library);
	};

	struct physical_device_features
	{
		bool robustBufferAccess : 1 = false;
		bool fullDrawIndexUint32 : 1 = false;
		bool imageCubeArray : 1 = false;
		bool independentBlend : 1 = false;
		bool geometryShader : 1 = false;
		bool tessellationShader : 1 = false;
		bool sampleRateShading : 1 = false;
		bool dualSrcBlend : 1 = false;
		bool logicOp : 1 = false;
		bool multiDrawIndirect : 1 = false;
		bool drawIndirectFirstInstance : 1 = false;
		bool depthClamp : 1 = false;
		bool depthBiasClamp : 1 = false;
		bool fillModeNonSolid : 1 = false;
		bool depthBounds : 1 = false;
		bool wideLines : 1 = false;
		bool largePoints : 1 = false;
		bool alphaToOne : 1 = false;
		bool multiViewport : 1 = false;
		bool samplerAnisotropy : 1 = false;
		bool textureCompressionETC2 : 1 = false;
		bool textureCompressionASTC_LDR : 1 = false;
		bool textureCompressionBC : 1 = false;
		bool occlusionQueryPrecise : 1 = false;
		bool pipelineStatisticsQuery : 1 = false;
		bool vertexPipelineStoresAndAtomics : 1 = false;
		bool fragmentStoresAndAtomics : 1 = false;
		bool shaderTessellationAndGeometryPointSize : 1 = false;
		bool shaderImageGatherExtended : 1 = false;
		bool shaderStorageImageExtendedFormats : 1 = false;
		bool shaderStorageImageMultisample : 1 = false;
		bool shaderStorageImageReadWithoutFormat : 1 = false;
		bool shaderStorageImageWriteWithoutFormat : 1 = false;
		bool shaderUniformBufferArrayDynamicIndexing : 1 = false;
		bool shaderSampledImageArrayDynamicIndexing : 1 = false;
		bool shaderStorageBufferArrayDynamicIndexing : 1 = false;
		bool shaderStorageImageArrayDynamicIndexing : 1 = false;
		bool shaderClipDistance : 1 = false;
		bool shaderCullDistance : 1 = false;
		bool shaderFloat64 : 1 = false;
		bool shaderInt64 : 1 = false;
		bool shaderInt16 : 1 = false;
		bool shaderResourceResidency : 1 = false;
		bool shaderResourceMinLod : 1 = false;
		bool sparseBinding : 1 = false;
		bool sparseResidencyBuffer : 1 = false;
		bool sparseResidencyImage2D : 1 = false;
		bool sparseResidencyImage3D : 1 = false;
		bool sparseResidency2Samples : 1 = false;
		bool sparseResidency4Samples : 1 = false;
		bool sparseResidency8Samples : 1 = false;
		bool sparseResidency16Samples : 1 = false;
		bool sparseResidencyAliased : 1 = false;
		bool variableMultisampleRate : 1 = false;
		bool inheritedQueries : 1 = false;
	};

	enum struct [[rythe_closed_enum]] physical_device_type : rsl::uint8
	{
		other,
		integratedGPU,
		discreteGPU,
		virtualGPU,
		CPU,
	};

	std::string_view to_string(physical_device_type type);

	enum struct [[rythe_closed_enum]] [[rythe_flag_enum]] sample_count_flags : rsl::uint8
	{
		sc1Bit = 1 << 0,
		sc2Bit = 1 << 1,
		sc4Bit = 1 << 2,
		sc8Bit = 1 << 3,
		sc16Bit = 1 << 4,
		sc32Bit = 1 << 5,
		sc64Bit = 1 << 6,
	};

	struct physical_device_limits
	{
		rsl::uint32 maxImageDimension1D;
		rsl::uint32 maxImageDimension2D;
		rsl::uint32 maxImageDimension3D;
		rsl::uint32 maxImageDimensionCube;
		rsl::uint32 maxImageArrayLayers;
		rsl::uint32 maxTexelBufferElements;
		rsl::uint32 maxUniformBufferRange;
		rsl::uint32 maxStorageBufferRange;
		rsl::uint32 maxPushConstantsSize;
		rsl::uint32 maxMemoryAllocationCount;
		rsl::uint32 maxSamplerAllocationCount;
		rsl::uint64 bufferImageGranularity;
		rsl::uint64 sparseAddressSpaceSize;
		rsl::uint32 maxBoundDescriptorSets;
		rsl::uint32 maxPerStageDescriptorSamplers;
		rsl::uint32 maxPerStageDescriptorUniformBuffers;
		rsl::uint32 maxPerStageDescriptorStorageBuffers;
		rsl::uint32 maxPerStageDescriptorSampledImages;
		rsl::uint32 maxPerStageDescriptorStorageImages;
		rsl::uint32 maxPerStageDescriptorInputAttachments;
		rsl::uint32 maxPerStageResources;
		rsl::uint32 maxDescriptorSetSamplers;
		rsl::uint32 maxDescriptorSetUniformBuffers;
		rsl::uint32 maxDescriptorSetUniformBuffersDynamic;
		rsl::uint32 maxDescriptorSetStorageBuffers;
		rsl::uint32 maxDescriptorSetStorageBuffersDynamic;
		rsl::uint32 maxDescriptorSetSampledImages;
		rsl::uint32 maxDescriptorSetStorageImages;
		rsl::uint32 maxDescriptorSetInputAttachments;
		rsl::uint32 maxVertexInputAttributes;
		rsl::uint32 maxVertexInputBindings;
		rsl::uint32 maxVertexInputAttributeOffset;
		rsl::uint32 maxVertexInputBindingStride;
		rsl::uint32 maxVertexOutputComponents;
		rsl::uint32 maxTessellationGenerationLevel;
		rsl::uint32 maxTessellationPatchSize;
		rsl::uint32 maxTessellationControlPerVertexInputComponents;
		rsl::uint32 maxTessellationControlPerVertexOutputComponents;
		rsl::uint32 maxTessellationControlPerPatchOutputComponents;
		rsl::uint32 maxTessellationControlTotalOutputComponents;
		rsl::uint32 maxTessellationEvaluationInputComponents;
		rsl::uint32 maxTessellationEvaluationOutputComponents;
		rsl::uint32 maxGeometryShaderInvocations;
		rsl::uint32 maxGeometryInputComponents;
		rsl::uint32 maxGeometryOutputComponents;
		rsl::uint32 maxGeometryOutputVertices;
		rsl::uint32 maxGeometryTotalOutputComponents;
		rsl::uint32 maxFragmentInputComponents;
		rsl::uint32 maxFragmentOutputAttachments;
		rsl::uint32 maxFragmentDualSrcAttachments;
		rsl::uint32 maxFragmentCombinedOutputResources;
		rsl::uint32 maxComputeSharedMemorySize;
		rsl::uint32 maxComputeWorkGroupCount[3];
		rsl::uint32 maxComputeWorkGroupInvocations;
		rsl::uint32 maxComputeWorkGroupSize[3];
		rsl::uint32 subPixelPrecisionBits;
		rsl::uint32 subTexelPrecisionBits;
		rsl::uint32 mipmapPrecisionBits;
		rsl::uint32 maxDrawIndexedIndexValue;
		rsl::uint32 maxDrawIndirectCount;
		rsl::float32 maxSamplerLodBias;
		rsl::float32 maxSamplerAnisotropy;
		rsl::uint32 maxViewports;
		rsl::uint32 maxViewportDimensions[2];
		rsl::float32 viewportBoundsRange[2];
		rsl::uint32 viewportSubPixelBits;
		rsl::size_type minMemoryMapAlignment;
		rsl::uint64 minTexelBufferOffsetAlignment;
		rsl::uint64 minUniformBufferOffsetAlignment;
		rsl::uint64 minStorageBufferOffsetAlignment;
		rsl::i32 minTexelOffset;
		rsl::uint32 maxTexelOffset;
		rsl::i32 minTexelGatherOffset;
		rsl::uint32 maxTexelGatherOffset;
		rsl::float32 minInterpolationOffset;
		rsl::float32 maxInterpolationOffset;
		rsl::uint32 subPixelInterpolationOffsetBits;
		rsl::uint32 maxFramebufferWidth;
		rsl::uint32 maxFramebufferHeight;
		rsl::uint32 maxFramebufferLayers;
		sample_count_flags framebufferColorSampleCounts;
		sample_count_flags framebufferDepthSampleCounts;
		sample_count_flags framebufferStencilSampleCounts;
		sample_count_flags framebufferNoAttachmentsSampleCounts;
		rsl::uint32 maxColorAttachments;
		sample_count_flags sampledImageColorSampleCounts;
		sample_count_flags sampledImageIntegerSampleCounts;
		sample_count_flags sampledImageDepthSampleCounts;
		sample_count_flags sampledImageStencilSampleCounts;
		sample_count_flags storageImageSampleCounts;
		rsl::uint32 maxSampleMaskWords;
		bool timestampComputeAndGraphics;
		rsl::float32 timestampPeriod;
		rsl::uint32 maxClipDistances;
		rsl::uint32 maxCullDistances;
		rsl::uint32 maxCombinedClipAndCullDistances;
		rsl::uint32 discreteQueuePriorities;
		rsl::float32 pointSizeRange[2];
		rsl::float32 lineWidthRange[2];
		rsl::float32 pointSizeGranularity;
		rsl::float32 lineWidthGranularity;
		bool strictLines;
		bool standardSampleLocations;
		rsl::uint64 optimalBufferCopyOffsetAlignment;
		rsl::uint64 optimalBufferCopyRowPitchAlignment;
		rsl::uint64 nonCoherentAtomSize;
	};

	struct physical_device_sparse_properties
	{
		bool residencyStandard2DBlockShape;
		bool residencyStandard2DMultisampleBlockShape;
		bool residencyStandard3DBlockShape;
		bool residencyAlignedMipSize;
		bool residencyNonResidentStrict;
	};

	struct physical_device_properties
	{
		semver::version apiVersion;
		semver::version driverVersion;
		rsl::uint32 vendorID;
		rsl::uint32 deviceID;
		physical_device_type deviceType;
		std::string deviceName;
		physical_device_limits limits;
		physical_device_sparse_properties sparseProperties;
	};

	struct physical_device_description
	{
		rsl::size_type deviceTypeImportance[5] = {
			0ull,    // Other
			100ull,  // Integrated
			1000ull, // Discrete
			0ull,    // Virtual
			0ull,    // CPU
		};
		semver::version apiVersion = semver::version(0, 0, 0);
		physical_device_features requiredFeatures = {};
		rsl::size_type requiredPerStageSampledImages = 4096;
	};

	enum struct [[rythe_closed_enum]] [[rythe_flag_enum]] queue_feature_flags : rsl::uint32
	{
		graphics = 1 << 0,
		compute = 1 << 1,
		transfer = 1 << 2,
		sparseBinding = 1 << 3,
		protectedMemory = 1 << 4,
		videoDecode = 1 << 5,
		videoEncode = 1 << 6,
		opticalFlowNV = 1 << 7,
		present = 1 << 8,
	};

	struct queue_family_properties
	{
		queue_feature_flags features;
		rsl::size_type queueCount;
		rsl::size_type timestampValidBits;
		rsl::math::uint3 minImageTransferGranularity;
	};

	enum struct [[rythe_closed_enum]] queue_priority : uint8_t
	{
		normal,
		high,
	};

	std::string_view to_string(queue_priority priority);

	struct queue_description
	{
		rsl::size_type queueFamilyIndexOverride = rsl::npos;
		queue_priority priority = queue_priority::normal;

		// For auto family selection
		queue_feature_flags requiredFeatures;
		rsl::size_type queueCountImportance = 8ull;
		rsl::size_type timestampImportance = 2ull;
		rsl::size_type imageTransferGranularityImportance = 1ull;
	};

	struct queue_family_selection
	{
		rsl::size_type familyIndex = rsl::npos;
		rsl::size_type score = 0;
	};

	enum struct [[rythe_closed_enum]] [[rythe_flag_enum]] surface_transform_flags : rsl::uint32
	{
		identity = 1 << 0,
		rotate90 = 1 << 1,
		rotate180 = 1 << 2,
		rotate270 = 1 << 3,
		horizontalMirror = 1 << 4,
		horizontalMirrorRotate90 = 1 << 5,
		horizontalMirrorRotate180 = 1 << 6,
		horizontalMirrorRotate270 = 1 << 7,
		inherit = 1 << 8
	};

	enum struct [[rythe_closed_enum]] [[rythe_flag_enum]] composite_alpha_flags : rsl::uint32
	{
		opaque = 1 << 0,
		preMultiplied = 1 << 1,
		postMultiplied = 1 << 2,
		inherit = 1 << 3
	};

	enum struct [[rythe_closed_enum]] [[rythe_flag_enum]] image_usage_flags : rsl::uint32
	{
		transferSrc = 1 << 0,
		transferDst = 1 << 1,
		sampled = 1 << 2,
		storage = 1 << 3,
		colorAttachment = 1 << 4,
		depthStencilAttachment = 1 << 5,
		transientAttachment = 1 << 6,
		inputAttachment = 1 << 7,
		fragmentShadingRateAttachment = 1 << 8,
		fragmentDensityMap = 1 << 9,
		videoDecodeDst = 1 << 10,
		videoDecodeSrc = 1 << 11,
		videoDecodeDpb = 1 << 12,
		videoEncodeDst = 1 << 13,
		videoEncodeSrc = 1 << 14,
		videoEncodeDpb = 1 << 15,
		invocationMaskHUAWEI = 1 << 18,
		attachmentFeedbackLoop = 1 << 19,
		sampleWeightQCOM = 1 << 20,
		sampleBlockMatchQCOM = 1 << 21,
		hostTransfer = 1 << 22
	};

	struct surface_capabilities
	{
		rsl::size_type minImageCount;
		rsl::size_type maxImageCount;
		rsl::math::uint2 currentExtent;
		rsl::math::uint2 minImageExtent;
		rsl::math::uint2 maxImageExtent;
		rsl::size_type maxImageArrayLayers;
		surface_transform_flags supportedTransforms;
		surface_transform_flags currentTransform;
		composite_alpha_flags supportedCompositeAlpha;
		image_usage_flags supportedUsageFlags;
	};

	class physical_device;
	class render_device;

	class surface
	{
	public:
		operator bool() const noexcept;

		void release();

		[[rythe_always_inline]] native_surface get_native_handle() const noexcept { return m_nativeSurface; }

	private:
		native_surface m_nativeSurface;

		friend void set_native_handle(surface&, native_surface);
	};

	class instance
	{
	public:
		operator bool() const noexcept;

		void release();

		std::span<physical_device> create_physical_devices(bool forceRefresh = false);
		void release_physical_devices();

		const application_info& get_application_info() const noexcept;
		const semver::version& get_api_version() const noexcept;

		[[nodiscard]] surface create_surface();

		[[nodiscard]] render_device auto_select_and_create_device(
			const physical_device_description& physicalDeviceDescription,
			std::span<const queue_description> queueDesciptions, surface surface = {},
			std::span<const rsl::hashed_string> extensions = {}
		);

		[[rythe_always_inline]] native_instance get_native_handle() const noexcept { return m_nativeInstance; }

	private:
		native_instance m_nativeInstance = invalid_native_instance;

		friend void set_native_handle(instance&, native_instance);
	};

	class physical_device
	{
	public:
		operator bool() const noexcept;

		void release();

		const surface_capabilities& get_surface_capabilities(surface _surface, bool forceRefresh = false);
		const physical_device_properties& get_properties(bool forceRefresh = false);
		const physical_device_features& get_features(bool forceRefresh = false);

		std::span<const extension_properties> get_available_extensions(bool forceRefresh = false);
		bool is_extension_available(rsl::hashed_string_view extensionName);

		std::span<const queue_family_properties>
		get_available_queue_families(surface surface = {}, bool forceRefresh = false);
		bool get_queue_family_selection(
			std::span<queue_family_selection> queueFamilySelections,
			std::span<const queue_description> queueDesciptions, surface surface = {}
		);

		bool in_use() const noexcept;

		[[nodiscard]] render_device create_render_device(
			std::span<const queue_description> queueDesciptions, std::span<const rsl::hashed_string> extensions = {}
		);

		[[rythe_always_inline]] native_physical_device get_native_handle() const noexcept
		{
			return m_nativePhysicalDevice;
		}

	private:
		native_physical_device m_nativePhysicalDevice = invalid_native_physical_device;

		friend void set_native_handle(physical_device&, native_physical_device);
	};

	class queue;

	class render_device
	{
	public:
		operator bool() const noexcept;

		void release();

		std::span<queue> get_queues() noexcept;
		physical_device get_physical_device() const noexcept;

		[[rythe_always_inline]] native_render_device get_native_handle() const noexcept { return m_nativeRenderDevice; }

	private:
		native_render_device m_nativeRenderDevice = invalid_native_render_device;
		friend void set_native_handle(render_device&, native_render_device);
	};

	class transient_command_pool;
	class persistent_command_pool;

	class queue
	{
	public:
		operator bool() const noexcept;

		void release();

		rsl::size_type get_index() const noexcept;
		rsl::size_type get_family_index() const noexcept;
		queue_priority get_priority() const noexcept;
		const queue_family_properties& get_family() const noexcept;

		[[nodiscard]] persistent_command_pool create_persistent_command_pool(bool protectedCommandBuffers = false);
		[[nodiscard]] transient_command_pool create_transient_command_pool(bool protectedCommandBuffers = false);

		[[rythe_always_inline]] native_queue get_native_handle() const noexcept { return m_nativeQueue; }

	private:
		native_queue m_nativeQueue = invalid_native_queue;
		friend void set_native_handle(queue&, native_queue);
	};

	enum struct [[rythe_closed_enum]] command_buffer_level : rsl::uint8
	{
		primary,
		secondary,
	};

	class command_buffer;

	class command_pool
	{
	public:
		operator bool() const noexcept;

		virtual void release() = 0;

		virtual rsl::size_type
		get_capacity(command_buffer_level level = command_buffer_level::primary) const noexcept = 0;
		virtual rsl::size_type
		get_unused_count(command_buffer_level level = command_buffer_level::primary) const noexcept = 0;

		virtual void reserve(rsl::size_type count, command_buffer_level level = command_buffer_level::primary) = 0;
		virtual command_buffer get_command_buffer(command_buffer_level level = command_buffer_level::primary) = 0;
		virtual void return_command_buffer(command_buffer& commandBuffer) = 0;

		[[rythe_always_inline]] native_command_pool get_native_handle() const noexcept { return m_nativeCommandPool; }

	protected:
		native_command_pool m_nativeCommandPool = invalid_native_command_pool;
		friend void set_native_handle(command_pool&, native_command_pool);
	};

	class persistent_command_pool : public command_pool
	{
	public:
		using command_pool::operator bool;

		void release() override;

		rsl::size_type get_capacity(command_buffer_level level = command_buffer_level::primary) const noexcept override;
		rsl::size_type
		get_unused_count(command_buffer_level level = command_buffer_level::primary) const noexcept override;

		void reserve(rsl::size_type count, command_buffer_level level = command_buffer_level::primary) override;
		command_buffer get_command_buffer(command_buffer_level level = command_buffer_level::primary) override;
		void return_command_buffer(command_buffer& commandBuffer) override;
	};

	class transient_command_pool : public command_pool
	{
	public:
		using command_pool::operator bool;

		void release() override;

		rsl::size_type get_capacity(command_buffer_level level = command_buffer_level::primary) const noexcept override;
		rsl::size_type
		get_unused_count(command_buffer_level level = command_buffer_level::primary) const noexcept override;

		void reserve(rsl::size_type count, command_buffer_level level = command_buffer_level::primary) override;
		command_buffer get_command_buffer(command_buffer_level level = command_buffer_level::primary) override;
		void return_command_buffer(command_buffer& commandBuffer) override;
	};

	class command_buffer
	{
	public:
		operator bool() const noexcept;

		void return_to_pool();

		[[rythe_always_inline]] native_command_buffer get_native_handle() const noexcept
		{
			return m_nativeCommandBuffer;
		}

	private:
		native_command_buffer m_nativeCommandBuffer = invalid_native_command_buffer;
		friend void set_native_handle(command_buffer&, native_command_buffer);
	};
} // namespace vk
