#pragma once

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

	class physical_device;

	class instance
	{
	public:
		operator bool() const noexcept;

		std::span<physical_device> get_physical_devices(bool forceRefresh = false);

		rythe_always_inline native_instance get_native_handle() const noexcept { return m_nativeInstance; }

	private:
		native_instance m_nativeInstance = invalid_native_instance;

		friend instance create_instance(
			const application_info& applicationInfo, const semver::version& apiVersion,
			std::span<rsl::cstring> extensions
		);
	};

	struct physical_device_features
	{
		bool robustBufferAccess;
		bool fullDrawIndexUint32;
		bool imageCubeArray;
		bool independentBlend;
		bool geometryShader;
		bool tessellationShader;
		bool sampleRateShading;
		bool dualSrcBlend;
		bool logicOp;
		bool multiDrawIndirect;
		bool drawIndirectFirstInstance;
		bool depthClamp;
		bool depthBiasClamp;
		bool fillModeNonSolid;
		bool depthBounds;
		bool wideLines;
		bool largePoints;
		bool alphaToOne;
		bool multiViewport;
		bool samplerAnisotropy;
		bool textureCompressionETC2;
		bool textureCompressionASTC_LDR;
		bool textureCompressionBC;
		bool occlusionQueryPrecise;
		bool pipelineStatisticsQuery;
		bool vertexPipelineStoresAndAtomics;
		bool fragmentStoresAndAtomics;
		bool shaderTessellationAndGeometryPointSize;
		bool shaderImageGatherExtended;
		bool shaderStorageImageExtendedFormats;
		bool shaderStorageImageMultisample;
		bool shaderStorageImageReadWithoutFormat;
		bool shaderStorageImageWriteWithoutFormat;
		bool shaderUniformBufferArrayDynamicIndexing;
		bool shaderSampledImageArrayDynamicIndexing;
		bool shaderStorageBufferArrayDynamicIndexing;
		bool shaderStorageImageArrayDynamicIndexing;
		bool shaderClipDistance;
		bool shaderCullDistance;
		bool shaderFloat64;
		bool shaderInt64;
		bool shaderInt16;
		bool shaderResourceResidency;
		bool shaderResourceMinLod;
		bool sparseBinding;
		bool sparseResidencyBuffer;
		bool sparseResidencyImage2D;
		bool sparseResidencyImage3D;
		bool sparseResidency2Samples;
		bool sparseResidency4Samples;
		bool sparseResidency8Samples;
		bool sparseResidency16Samples;
		bool sparseResidencyAliased;
		bool variableMultisampleRate;
		bool inheritedQueries;
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

	class render_device;

    DECLARE_OPAQUE_HANDLE(native_physical_device);

	class physical_device
	{
	public:
		operator bool() const noexcept;

		const physical_device_properties& get_properties(bool forceRefresh = false);
		const physical_device_features& get_features(bool forceRefresh = false);
		std::span<const extension_properties> get_available_extensions(bool forceRefresh = false);
		std::span<const queue_family_properties> get_available_queue_families(bool forceRefresh = false);

		rythe_always_inline native_physical_device get_native_handle() const noexcept { return m_nativePhysicalDevice; }

		bool initialize(std::span<rsl::cstring> extensions);

		render_device create_render_device(std::span<const queue_description> queueDesciptions);

	private:
		native_physical_device m_nativePhysicalDevice = invalid_native_physical_device;
		friend class instance;
	};

    DECLARE_OPAQUE_HANDLE(native_render_device);

	class render_device
	{
	public:
		operator bool() const noexcept;

		rythe_always_inline native_render_device get_native_handle() const noexcept { return m_nativeRenderDevice; }

    private:
		native_render_device m_nativeRenderDevice = invalid_native_render_device;
		friend class physical_device;
	};
} // namespace vk
