#include <iostream>

#include <rsl/utilities>

#include "vk/vulkan.hpp"

#if RYTHE_PLATFORM_WINDOWS
	#include <Windows.h>

LRESULT
Wndproc(
	[[maybe_unused]] HWND window, [[maybe_unused]] UINT message, [[maybe_unused]] WPARAM wparam,
	[[maybe_unused]] LPARAM lparam
)
{
	if (message == WM_CLOSE)
	{
		PostQuitMessage(NO_ERROR);
		return NO_ERROR;
	}

	return DefWindowProc(window, message, wparam, lparam);
}

#endif

int main()
{
	rsl::default_pmu_allocator allocator;
	vk::graphics_library lib = vk::init(allocator);

	if (!lib)
	{
		std::cout << "Failed to initialize vulkan\n";
		return -1;
	}

	std::cout << "Available Instance layers:\n";
	for (auto& layer : lib.get_available_instance_layers())
	{
		std::cout << '\t' << layer.name.c_str() << " [" << layer.specVersion << "] [" << layer.implementationVersion
				  << "]\n\t\t" << layer.description << "\n";
	}

	std::cout << '\n';

	std::cout << "Available Instance extensions:\n";
	for (auto& extension : lib.get_available_instance_extensions())
	{
		std::cout << '\t' << extension.name.c_str() << " [" << extension.specVersion << "]\n";
	}

	std::cout << '\n';

	vk::native_window_handle windowHandle = vk::invalid_native_window_handle;

#if RYTHE_PLATFORM_WINDOWS
	WCHAR CLASS_NAME[] = L"Learning-VK Window Class";
	HINSTANCE hinstance = GetModuleHandleA(nullptr);

	WNDCLASS wc = {};

	wc.lpfnWndProc = Wndproc;
	wc.hInstance = hinstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClassW(&wc);

	HWND hwnd = CreateWindowW(
		CLASS_NAME, L"Learning-VK", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		nullptr, nullptr, hinstance, nullptr
	);

	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	vk::native_window_info_win32 windowInfo{
        .alloc = allocator,
		.hinstance = hinstance,
		.hwnd = hwnd,
	};

	windowHandle = vk::create_window_handle_win32(windowInfo);
#endif

	vk::application_info applicationInfo{
		.name = "Learning VK",
		.version = {0, 0, 1},
		.windowHandle = windowHandle,
	};

	vk::instance instance = lib.create_instance(applicationInfo);
	if (!instance)
	{
		return -1;
	}

	vk::physical_device_description deviceDesc;

	vk::queue_description queueDescs[] = {
		{
		 .priority = vk::queue_priority::high,
		 .requiredFeatures = vk::queue_feature_flags::graphics,
		 },
		{
		 .requiredFeatures = vk::queue_feature_flags::compute,
		 },
		{
		 .requiredFeatures = vk::queue_feature_flags::transfer,
		 },
		{
		 .requiredFeatures = vk::queue_feature_flags::present,
		 },
	};

	auto surface = instance.create_surface();

	auto renderDevice = instance.auto_select_and_create_device(deviceDesc, queueDescs, surface);

	if (!renderDevice)
	{
		std::cout << "NO DEVICE FOUND\n";
		return -1;
	}

	auto physicalDevice = renderDevice.get_physical_device();

	{
		auto& properties = physicalDevice.get_properties();

		std::cout << "Device:\n";
		std::cout << "\tapi version: [" << properties.apiVersion << "]\n";
		std::cout << "\tdriver version: [" << properties.driverVersion << "]\n";
		std::cout << "\tvendor ID: " << properties.vendorID << "\n";
		std::cout << "\tdevice ID: " << properties.deviceID << "\n";
		std::cout << "\tdevice type: " << vk::to_string(properties.deviceType) << "\n";
		std::cout << "\tdevice name: \"" << properties.deviceName << "\"\n";

		auto& surfaceCapabilities = physicalDevice.get_surface_capabilities(surface);

		std::cout << "\tsurface capabilities:\n";

		std::cout << "\t\tminimum image count: " << surfaceCapabilities.minImageCount << '\n';
		std::cout << "\t\tmaximum image count: " << surfaceCapabilities.maxImageCount << '\n';
		std::cout << "\t\tcurrent extent:\n";
		std::cout << "\t\t\twidth: " << surfaceCapabilities.currentExtent.x << '\n';
		std::cout << "\t\t\theight: " << surfaceCapabilities.currentExtent.y << '\n';
		std::cout << "\t\tminimum image extent:\n";
		std::cout << "\t\t\twidth: " << surfaceCapabilities.minImageExtent.x << '\n';
		std::cout << "\t\t\theight: " << surfaceCapabilities.minImageExtent.y << '\n';
		std::cout << "\t\tmaximum image extent:\n";
		std::cout << "\t\t\twidth: " << surfaceCapabilities.maxImageExtent.x << '\n';
		std::cout << "\t\t\theight: " << surfaceCapabilities.maxImageExtent.y << '\n';
		std::cout << "\t\tmaximum array layers: " << surfaceCapabilities.maxImageArrayLayers << '\n';


#define PRINT_FLAG(flags, flagtype, name)                                                                              \
	if (rsl::enum_flags::has_flag(surfaceCapabilities.flags, flagtype))                                                \
	{                                                                                                                  \
		std::cout << "\t\t\t" name "\n";                                                                               \
	}

		std::cout << "\t\tsupported transforms:\n";
		PRINT_FLAG(supportedTransforms, vk::surface_transform_flags::identity, "identity");
		PRINT_FLAG(supportedTransforms, vk::surface_transform_flags::rotate90, "rotate 90");
		PRINT_FLAG(supportedTransforms, vk::surface_transform_flags::rotate180, "rotate 180");
		PRINT_FLAG(supportedTransforms, vk::surface_transform_flags::rotate270, "rotate 270");
		PRINT_FLAG(supportedTransforms, vk::surface_transform_flags::horizontalMirror, "horizontal mirror");
		PRINT_FLAG(
			supportedTransforms, vk::surface_transform_flags::horizontalMirrorRotate90, "horizontal mirror rotate 90"
		);
		PRINT_FLAG(
			supportedTransforms, vk::surface_transform_flags::horizontalMirrorRotate180, "horizontal mirror rotate 180"
		);
		PRINT_FLAG(
			supportedTransforms, vk::surface_transform_flags::horizontalMirrorRotate270, "horizontal mirror rotate 270"
		);
		PRINT_FLAG(supportedTransforms, vk::surface_transform_flags::inherit, "inherit");

		std::cout << "\t\tcurrent transform:\n";
		PRINT_FLAG(currentTransform, vk::surface_transform_flags::identity, "identity");
		PRINT_FLAG(currentTransform, vk::surface_transform_flags::rotate90, "rotate 90");
		PRINT_FLAG(currentTransform, vk::surface_transform_flags::rotate180, "rotate 180");
		PRINT_FLAG(currentTransform, vk::surface_transform_flags::rotate270, "rotate 270");
		PRINT_FLAG(currentTransform, vk::surface_transform_flags::horizontalMirror, "horizontal mirror");
		PRINT_FLAG(
			currentTransform, vk::surface_transform_flags::horizontalMirrorRotate90, "horizontal mirror rotate 90"
		);
		PRINT_FLAG(
			currentTransform, vk::surface_transform_flags::horizontalMirrorRotate180, "horizontal mirror rotate 180"
		);
		PRINT_FLAG(
			currentTransform, vk::surface_transform_flags::horizontalMirrorRotate270, "horizontal mirror rotate 270"
		);
		PRINT_FLAG(currentTransform, vk::surface_transform_flags::inherit, "inherit");

		std::cout << "\t\tsupported composite alpha:\n";
		PRINT_FLAG(supportedCompositeAlpha, vk::composite_alpha_flags::opaque, "opaque");
		PRINT_FLAG(supportedCompositeAlpha, vk::composite_alpha_flags::preMultiplied, "pre multiplied");
		PRINT_FLAG(supportedCompositeAlpha, vk::composite_alpha_flags::postMultiplied, "post multiplied");
		PRINT_FLAG(supportedCompositeAlpha, vk::composite_alpha_flags::inherit, "inherit");

		std::cout << "\t\tsupported image usage:\n";

		PRINT_FLAG(supportedUsageFlags, vk::image_usage_flags::transferSrc, "transfer src");
		PRINT_FLAG(supportedUsageFlags, vk::image_usage_flags::transferDst, "transfer dst");
		PRINT_FLAG(supportedUsageFlags, vk::image_usage_flags::sampled, "sampled");
		PRINT_FLAG(supportedUsageFlags, vk::image_usage_flags::storage, "storage");
		PRINT_FLAG(supportedUsageFlags, vk::image_usage_flags::colorAttachment, "color attachment");
		PRINT_FLAG(supportedUsageFlags, vk::image_usage_flags::depthStencilAttachment, "depth stencil attachment");
		PRINT_FLAG(supportedUsageFlags, vk::image_usage_flags::transientAttachment, "transient attachment");
		PRINT_FLAG(supportedUsageFlags, vk::image_usage_flags::inputAttachment, "input attachment");
		PRINT_FLAG(
			supportedUsageFlags, vk::image_usage_flags::fragmentShadingRateAttachment,
			"fragment shading rate attachment"
		);
		PRINT_FLAG(supportedUsageFlags, vk::image_usage_flags::fragmentDensityMap, "fragment density map");
		PRINT_FLAG(supportedUsageFlags, vk::image_usage_flags::videoDecodeDst, "video decode dst");
		PRINT_FLAG(supportedUsageFlags, vk::image_usage_flags::videoDecodeSrc, "video decode src");
		PRINT_FLAG(supportedUsageFlags, vk::image_usage_flags::videoDecodeDpb, "video decode dpb");
		PRINT_FLAG(supportedUsageFlags, vk::image_usage_flags::videoEncodeDst, "video encode dst");
		PRINT_FLAG(supportedUsageFlags, vk::image_usage_flags::videoEncodeSrc, "video encode src");
		PRINT_FLAG(supportedUsageFlags, vk::image_usage_flags::videoEncodeDpb, "video encode dpb");
		PRINT_FLAG(supportedUsageFlags, vk::image_usage_flags::invocationMaskHUAWEI, "invocation mask HUAWEI");
		PRINT_FLAG(supportedUsageFlags, vk::image_usage_flags::attachmentFeedbackLoop, "attachment feedback loop");
		PRINT_FLAG(supportedUsageFlags, vk::image_usage_flags::sampleWeightQCOM, "sample weight QCOM");
		PRINT_FLAG(supportedUsageFlags, vk::image_usage_flags::sampleBlockMatchQCOM, "sample block match QCOM");
		PRINT_FLAG(supportedUsageFlags, vk::image_usage_flags::hostTransfer, "host transfer");

		auto& features = physicalDevice.get_features();

		std::cout << "\tfeatures:\n";
#define PRINT_FEATURE(name)                                                                                            \
	if (features.name)                                                                                                 \
	{                                                                                                                  \
		std::cout << "\t\t" #name ": yes\n";                                                                           \
	}                                                                                                                  \
	else                                                                                                               \
	{                                                                                                                  \
		std::cout << "\t\t" #name ": no\n";                                                                            \
	}

		PRINT_FEATURE(robustBufferAccess);
		PRINT_FEATURE(fullDrawIndexUint32);
		PRINT_FEATURE(imageCubeArray);
		PRINT_FEATURE(independentBlend);
		PRINT_FEATURE(geometryShader);
		PRINT_FEATURE(tessellationShader);
		PRINT_FEATURE(sampleRateShading);
		PRINT_FEATURE(dualSrcBlend);
		PRINT_FEATURE(logicOp);
		PRINT_FEATURE(multiDrawIndirect);
		PRINT_FEATURE(drawIndirectFirstInstance);
		PRINT_FEATURE(depthClamp);
		PRINT_FEATURE(depthBiasClamp);
		PRINT_FEATURE(fillModeNonSolid);
		PRINT_FEATURE(depthBounds);
		PRINT_FEATURE(wideLines);
		PRINT_FEATURE(largePoints);
		PRINT_FEATURE(alphaToOne);
		PRINT_FEATURE(multiViewport);
		PRINT_FEATURE(samplerAnisotropy);
		PRINT_FEATURE(textureCompressionETC2);
		PRINT_FEATURE(textureCompressionASTC_LDR);
		PRINT_FEATURE(textureCompressionBC);
		PRINT_FEATURE(occlusionQueryPrecise);
		PRINT_FEATURE(pipelineStatisticsQuery);
		PRINT_FEATURE(vertexPipelineStoresAndAtomics);
		PRINT_FEATURE(fragmentStoresAndAtomics);
		PRINT_FEATURE(shaderTessellationAndGeometryPointSize);
		PRINT_FEATURE(shaderImageGatherExtended);
		PRINT_FEATURE(shaderStorageImageExtendedFormats);
		PRINT_FEATURE(shaderStorageImageMultisample);
		PRINT_FEATURE(shaderStorageImageReadWithoutFormat);
		PRINT_FEATURE(shaderStorageImageWriteWithoutFormat);
		PRINT_FEATURE(shaderUniformBufferArrayDynamicIndexing);
		PRINT_FEATURE(shaderSampledImageArrayDynamicIndexing);
		PRINT_FEATURE(shaderStorageBufferArrayDynamicIndexing);
		PRINT_FEATURE(shaderStorageImageArrayDynamicIndexing);
		PRINT_FEATURE(shaderClipDistance);
		PRINT_FEATURE(shaderCullDistance);
		PRINT_FEATURE(shaderFloat64);
		PRINT_FEATURE(shaderInt64);
		PRINT_FEATURE(shaderInt16);
		PRINT_FEATURE(shaderResourceResidency);
		PRINT_FEATURE(shaderResourceMinLod);
		PRINT_FEATURE(sparseBinding);
		PRINT_FEATURE(sparseResidencyBuffer);
		PRINT_FEATURE(sparseResidencyImage2D);
		PRINT_FEATURE(sparseResidencyImage3D);
		PRINT_FEATURE(sparseResidency2Samples);
		PRINT_FEATURE(sparseResidency4Samples);
		PRINT_FEATURE(sparseResidency8Samples);
		PRINT_FEATURE(sparseResidency16Samples);
		PRINT_FEATURE(sparseResidencyAliased);
		PRINT_FEATURE(variableMultisampleRate);
		PRINT_FEATURE(inheritedQueries);

#undef PRINT_FEATURE

		std::cout << "\tavailable extensions:\n";
		for (auto& extension : physicalDevice.get_available_extensions())
		{
			std::cout << "\t\t" << extension.name.c_str() << " [" << extension.specVersion << "]\n ";
		}

		std::cout << "\tavailable queue families:\n";
		rsl::size_type i = 0;
		for (auto& queueFamily : physicalDevice.get_available_queue_families())
		{
			std::cout << "\t\t" << i << ":\n";
			std::cout << "\t\t\tallowed operations:\n";

#define PRINT_FEATURE(feature, name)                                                                                   \
	if (rsl::enum_flags::has_flag(queueFamily.features, vk::queue_feature_flags::feature))                             \
	{                                                                                                                  \
		std::cout << "\t\t\t\t" name "\n";                                                                             \
	}

			PRINT_FEATURE(graphics, "graphics");
			PRINT_FEATURE(compute, "compute");
			PRINT_FEATURE(transfer, "transfer");
			PRINT_FEATURE(sparseBinding, "sparse binding");
			PRINT_FEATURE(protectedMemory, "protected memory");
			PRINT_FEATURE(videoDecode, "video decode");
			PRINT_FEATURE(videoEncode, "video encode");
			PRINT_FEATURE(opticalFlowNV, "optical flow");
			PRINT_FEATURE(present, "present");

#undef PRINT_FEATURE

			std::cout << "\t\t\tallowed amount: " << queueFamily.queueCount << '\n';
			std::cout << "\t\t\ttimestamp valid bits: " << queueFamily.timestampValidBits << '\n';
			std::cout << "\t\t\tminimum image transfer resolution:\n";
			std::cout << "\t\t\t\twidth: " << queueFamily.minImageTransferGranularity.x << '\n';
			std::cout << "\t\t\t\theight: " << queueFamily.minImageTransferGranularity.y << '\n';
			std::cout << "\t\t\t\tdepth: " << queueFamily.minImageTransferGranularity.z << '\n';

			i++;
		}
	}

	auto queues = renderDevice.get_queues();

	for (auto& queue : queues)
	{
		std::cout << "Queue:\n";
		std::cout << "\tindex: " << queue.get_index() << '\n';
		std::cout << "\tfamily index: " << queue.get_family_index() << '\n';
		std::cout << "\tpriority: " << vk::to_string(queue.get_priority()) << '\n';
	}

	auto graphicsQueue = queues[0];
	auto computeQueue = queues[1];
	auto transferQueue = queues[2];
	auto presentQueue = queues[3];

	auto presentCommandPool = presentQueue.create_persistent_command_pool();

	auto presentCommandBuffer = presentCommandPool.get_command_buffer();

    std::cout << (presentCommandBuffer ? "Command buffer created!\n" : "Command buffer failed to be created...\n");

#if RYTHE_PLATFORM_WINDOWS
	MSG message;
	while (GetMessage(&message, NULL, 0, 0) > 0)
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	};
#endif

	presentCommandBuffer.return_to_pool();

	presentCommandPool.release();

	graphicsQueue.release();
	computeQueue.release();
	transferQueue.release();
	presentQueue.release();

	renderDevice.release();

	surface.release();

	instance.release();

	vk::release_window_handle(windowHandle);

	lib.release();

#if RYTHE_PLATFORM_WINDOWS
	DestroyWindow(hwnd);
#endif

	std::cout << "Everything fine so far!\n";
	return 0;
}
