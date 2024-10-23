#pragma once

#include <rsl/primitives>
#include <rsl/math>
#include <rsl/platform>

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

	DECLARE_OPAQUE_HANDLE(native_instance);

	class instance;

	instance create_instance(
		const application_info& applicationInfo, const semver::version& apiVersion = {1, 0, 0},
		std::span<rsl::cstring> extensions = {}
	);

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

	enum struct physical_device_type : rsl::uint8
	{
		Other,
		Integrated,
		Discrete,
		Virtual,
		CPU,
	};

	std::string_view to_string(physical_device_type type);

	enum struct sample_count_flags : rsl::uint8
	{
		SC1Bit = 0x00000001,
		SC2Bit = 0x00000002,
		SC4Bit = 0x00000004,
		SC8Bit = 0x00000008,
		SC16Bit = 0x00000010,
		SC32Bit = 0x00000020,
		SC64Bit = 0x00000040,
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
            0ull, // Other
			100ull,  // Integrated
			1000ull, // Discrete
			0ull,    // Virtual
			0ull,    // CPU
        };
		semver::version apiVersion = semver::version(0,0,0);
		physical_device_features requiredFeatures;
	};

	enum struct queue_feature_flags : rsl::uint32
	{
		Graphics = 0x00000001,
		Compute = 0x00000002,
		Transfer = 0x00000004,
		SparseBinding = 0x00000008,
		Protected = 0x00000010,
		VideoDecode = 0x00000020,
		VideoEncode = 0x00000040,
		OpticalFlowNV = 0x00000100,
	};

	struct queue_family_properties
	{
		queue_feature_flags features;
		rsl::size_type queueCount;
		rsl::size_type timestampValidBits;
		rsl::math::uint3 minImageTransferGranularity;
	};

	enum struct queue_priority : uint8_t
	{
		Normal,
		High,
	};

	std::string_view to_string(queue_priority priority);

	struct queue_description
	{
		rsl::size_type queueFamilyIndexOverride = -1ull;
		queue_priority priority = queue_priority::Normal;

		// For auto family selection
		queue_feature_flags requiredFeatures;
		rsl::size_type queueCountImportance = 8ull;
		rsl::size_type timestampImportance = 2ull;
		rsl::size_type imageTransferGranularityImportance = 1ull;
	};

	class physical_device;
	class render_device;

	class instance
	{
	public:
		operator bool() const noexcept;

        void release();

		std::span<physical_device> create_physical_devices(bool forceRefresh = false);
		void release_physical_devices();

		rythe_always_inline native_instance get_native_handle() const noexcept { return m_nativeInstance; }

		render_device auto_select_and_create_device(
			const physical_device_description& physicalDeviceDescription,
			std::span<const queue_description> queueDesciptions, std::span<rsl::cstring> extensions = {}
		);

	private:
		native_instance m_nativeInstance = invalid_native_instance;

		friend instance create_instance(
			const application_info& applicationInfo, const semver::version& apiVersion,
			std::span<rsl::cstring> extensions
		);
	};

	DECLARE_OPAQUE_HANDLE(native_physical_device);

	class physical_device
	{
	public:
		operator bool() const noexcept;

        void release();

		const physical_device_properties& get_properties(bool forceRefresh = false);
		const physical_device_features& get_features(bool forceRefresh = false);
		std::span<const extension_properties> get_available_extensions(bool forceRefresh = false);
		bool is_extension_available(std::string_view extensionName);
		std::span<const queue_family_properties> get_available_queue_families(bool forceRefresh = false);

		rythe_always_inline native_physical_device get_native_handle() const noexcept { return m_nativePhysicalDevice; }

        bool in_use() const noexcept;

		render_device create_render_device(
			std::span<const queue_description> queueDesciptions, std::span<rsl::cstring> extensions = {}
		);

	private:
		render_device create_render_device_no_extension_check(
			std::span<const queue_description> queueDesciptions, std::span<rsl::cstring> extensions
		);

		native_physical_device m_nativePhysicalDevice = invalid_native_physical_device;
		friend physical_device copy_physical_device(physical_device);
		friend class instance;
	};

	DECLARE_OPAQUE_HANDLE(native_render_device);

	class render_device
	{
	public:
		operator bool() const noexcept;

        void release();

        physical_device get_physical_device() const noexcept;

		rythe_always_inline native_render_device get_native_handle() const noexcept { return m_nativeRenderDevice; }

	private:
		native_render_device m_nativeRenderDevice = invalid_native_render_device;
		friend class physical_device;
	};
} // namespace vk
