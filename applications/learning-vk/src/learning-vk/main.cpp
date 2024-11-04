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

int wmain()
{
	if (!vk::init())
	{
		std::cout << "Failed to initialize vulkan\n";
		return -1;
	}

	std::cout << "Available Instance extensions:\n";
	for (auto& extension : vk::get_available_instance_extensions())
	{
		std::cout << '\t' << extension.name << " [" << extension.specVersion << "]\n";
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
		CLASS_NAME, L"Learning-VK", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, hinstance, nullptr
	);

	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	vk::native_window_info_win32 windowInfo{
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

	vk::instance instance = vk::create_instance(applicationInfo);
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

	{
		auto device = renderDevice.get_physical_device();
		auto& properties = device.get_properties();

		std::cout << "Device:\n";
		std::cout << "\tapi version: [" << properties.apiVersion << "]\n";
		std::cout << "\tdriver version: [" << properties.driverVersion << "]\n";
		std::cout << "\tvendor ID: " << properties.vendorID << "\n";
		std::cout << "\tdevice ID: " << properties.deviceID << "\n";
		std::cout << "\tdevice type: " << vk::to_string(properties.deviceType) << "\n";
		std::cout << "\tdevice name: \"" << properties.deviceName << "\"\n";

		auto& features = device.get_features();

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
		for (auto& extension : device.get_available_extensions())
		{
			std::cout << "\t\t" << extension.name << " [" << extension.specVersion << "]\n ";
		}

		std::cout << "\tavailable queue families:\n";
		rsl::size_type i = 0;
		for (auto& queueFamily : device.get_available_queue_families())
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

	[[maybe_unused]] auto graphicsQueue = queues[0];
	[[maybe_unused]] auto computeQueue = queues[1];
	[[maybe_unused]] auto transferQueue = queues[2];

    #if RYTHE_PLATFORM_WINDOWS
	MSG message;
	while (GetMessage(&message, NULL, 0, 0) > 0)
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	};
    #endif

	renderDevice.release();

    surface.release();

	instance.release();

    vk::release_window_handle(windowHandle);

	vk::shut_down();

    #if RYTHE_PLATFORM_WINDOWS
	DestroyWindow(hwnd);
    #endif

	std::cout << "Everything fine so far!\n";
	return 0;
}
