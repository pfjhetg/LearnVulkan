#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <optional>
#include <set>
#include <fstream>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

// validation layers need to be enabled by specifying their name
// 和扩展一样，验证层也是通过指定层的名字来开启。Vulkan提供的标准的验证层被包装到一个名字为VK_LAYER_KHRONOS_validation的层中，这里我们就是开启vulkan提供的验证层。
const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// 从instance中寻找vkCreateDebugUtilsMessengerEXT方法的地址，如果开启了对应的层（VK_EXT_DEBUG_UTILS_EXTENSION_NAME），那么就会找到对应方法，否者返回nullptr。
// 因为vkCreateDebugUtilsMessengerEXT方法是在一个扩展中的，要调用它只能通过vkGetInstanceProcAddr来找到vkCreateDebugUtilsMessengerEXT这个方法
// 然后进行调用（个人理解类似反射)
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
	const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

// 销毁 原理同上面差不多
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

// 这个结构体是为了更方便而封装的，其optional类型是一种很好的判断没有赋值的方式；
// 增加了一个是否支持present的判断。
struct QueueFamilyIndices {
	std::optional<uint32_t>graphicsFamily;
	// 表示支持surface的present
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

// 这个结构体和QeueuFamilyIndices作用原理类似，用于判断交换链需要的三个属性。
struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

// 交换链需要支持的扩展
const std::vector<const char *> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

class HelloTriangleApplication {
public:
	void run() {
		// 初始化GLFW
		initWindow();
		// 初始化Vulkan
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	GLFWwindow* window;
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	// 属于WSI (Window System Integration) extensions.，VK_KHR_surface，包含在glfwGetRequiredInstanceExtensions中。这个实例是和平台无关的
	VkSurfaceKHR surface;
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	void initWindow() {
		glfwInit();

		// 因为默认是给OpenGL用的，这里不需要因此用GLFW_NO_API
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// 关闭缩放窗口
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	}

	void initVulkan() {
		// 创建实例 必须步骤
		createInstance();
		// 设置debug 可选
		setupDebugMessenger();
		// 放在这里是因为这一步会影响pickPhysicalDevice。(教程中这一步是在创建逻辑设备后面的) 必须步骤
		createSurface();
		// 选择物理设备 必须步骤
		pickPhysicalDevice();
		// 创建逻辑设备 必须步骤
		createLogicalDevice();
		// 根据前面的一些准备，然后在这里创建交换链 必须步骤
		createSwapChain();
		// 必须步骤
		createImageViews();
		// 必须步骤
		createRenderPass();
		// 必须步骤
		createGraphicsPipeline();
		// 必须步骤
		createFramebuffers();
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}
	}

	void cleanup() {
		for (auto framebuffer : swapChainFramebuffers) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}

		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);

		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(device, imageView, nullptr);
		}

		vkDestroySwapchainKHR(device, swapChain, nullptr);
		vkDestroyDevice(device, nullptr);

		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}

		// destroyed before the instance.
		vkDestroySurfaceKHR(instance, surface, nullptr);
		// 这个实例在清理的时候需要自己销毁
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);

		glfwTerminate();
	}

	// 创建Vulkan实例，用于连接应用和Vulkan库
	void createInstance() {
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available!");
		}
		VkInstanceCreateInfo createInfo{};
		// 可以从这个方法作用域尾部的代码开始往前读来理解，首先查阅API看需要那几个参数，然后一一进行填充。
		// 1.要创建一个实例，需要指定一些列参数
		// 2.如果启用了验证层，就开启我们增加的验证层
		// 3.扩展数量和对应的扩展名字是通过上面部分专门的方法获取（vulkan很多地方都是通过这样类似方式获取的，一定要熟悉这种方式）
		// 4.pApplicationInfo参数需要一个新的对象，因此就有了代码最前面部分的VkApplicationInfo

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		/*	uint32_t glfwExtensionCount = 0;
			const char **glfwExtensions;
			glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

			createInfo.enabledExtensionCount = glfwExtensionCount;
			createInfo.ppEnabledExtensionNames = glfwExtensions;*/
			// 用新的获取扩展的方式替代了上面旧的
		auto extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		// 这个debugCreateInfo是用来在vkCreateInstance和vkDestroyInstance之间调试的。
		// 前面章节的CreateDebugUtilsMessengerEXT和DestroyDebugUtilsMessengerEXT分别在vkCreateInstance前和vkDestroyInstance后
		// 因此无法调试vkCreateInstance和vkDestroyInstance之间的任何问题。
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if (enableValidationLayers) {// 添加要启用的层
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			// 这个相关知识可以参考https://github.com/KhronosGroup/Vulkan-Docs/blob/master/appendices/VK_EXT_debug_utils.txt#L120中的
			// 84行附近的代码，注释特别说明了这样的放的callback只会在vkCreateInstance和vkDestroyInstance之间触发
			// 这里这样设置了，不用专门调用vkCreateDebugUtilsMessengerEXT和vkDestroyDebugUtilsMessengerEXT了，内部处理了会自动启用，并且生命周期是跟着createInfo的。
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		// 最后创建实例
		// 第一个参数就是我们这里用到的创建信息，第二个是回调，这里不关注留空，第三个就是创建后的Handle
		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}

	// 创建调式信使(直译，理解形态就可以), 步骤和createInstance比较类似，因此可以用同样的理解步骤来帮助理解这个。
	void setupDebugMessenger() {
		if (!enableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		/*createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallBack;*/
		// 填充createInfo参数
		populateDebugMessengerCreateInfo(createInfo);
		// 可选的，这里我们不需要
		createInfo.pUserData = nullptr;

		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	// 这部分代码分离出来是为了增加在vkCreateInstance和vkDestroyInstance生命周期内的debug。
	// （这里的& 表示引用，引用就是某一变量（目标）的一个别名，对引用的操作与对变量直接操作完全一样。遗忘的c++知识需要及时复习）
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallBack;
	}

	bool checkValidationLayerSupport() {
		uint32_t layerCount;
		// 后面参数为nullptr时候，是获取层的数量
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		// availableLayers用来存放系统支持的层
		std::vector<VkLayerProperties> availableLayers(layerCount);
		// 后面参数不为nullptr时候，表示遍及层，然后依次赋值。
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		//检测我们开启的验证层是否在系统支持的所有可用的层中
		for (const char *layerName : validationLayers) {
			bool layerFound = false;

			for (const auto &layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}

		return true;
	}

	// 封装了一下获取扩展的方法，glfw包含的扩展是一直需要的，debug messenger扩展是根据是否开启了验证层来加入的。               
	// 前面一章是直接把glfw包含的扩展当成createInfo需要的信息了，这里增加了一个可选的debug messenger扩展，因此有必要重新封装一下。
	std::vector<const char *> getRequiredExtensions() {
		uint32_t glfwExtensionsCount = 0;
		const char **glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
		// 从化glfwExtensions第0个元素始，到glfwExtensionsCount个元素止的元素来初始化Vector（这是std::vector的初始化方式，忘记了可以Google）
		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionsCount);

		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		return extensions;
	}

	// The VKAPI_ATTR and VKAPI_CALL ensure that the function has the right signature for Vulkan to call it
	// 第一个参数是消息的严重程度
	// 第二个参数是事件类型
	// 第三个参数是详细的信息
	// 第四个参数是让用户可以自定义数据的
	// If the callback returns true, then the call is aborted with the VK_ERROR_VALIDATION_FAILED_EXT error.
	// 这里只是测试验证层，所以返回VK_FALAE即可
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallBack(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
		return VK_FALSE;
	}

	void pickPhysicalDevice() {
		uint32_t deviceCount = 0;
		// 这里遍历所有支持的硬件设备
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		if (deviceCount == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		// 这里只是一个教程，不做过多处理。因此找到一个适合的就可以（也可以根据教程说得按照需求自己计算一个得分来根据需求取）
		for (const auto &device : devices) {
			if (isDeviceSuitable(device)) {
				physicalDevice = device;
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	// 判断一个硬件设备是否适合(首先判断队列是否支持，然后判断是否支持指定的扩展，最后判断交换链支持情况)
	bool isDeviceSuitable(VkPhysicalDevice device) {
		// findQueueFamilies中的查找是基于设备中的队列族（queueFamilyes）来分类进行的
		QueueFamilyIndices indices = findQueueFamilies(device);
		bool extensionsSupported = checkDeviceExtensionSupport(device);
		// 判断details of swap chain support
		bool swapChainAdequate = false;
		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupportDetails = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupportDetails.formats.empty() && !swapChainSupportDetails.presentModes.empty();
		}

		return indices.isComplete() && extensionsSupported && swapChainAdequate;
	}

	// 判断硬件是否支持deviceExtensions中的扩展
	bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
		// 取到硬件支持的扩展
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		// 我们的程序中需要用到的扩展
		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
		// 遍历支持的扩展，如果里面的某一个包含到了需要的扩展中，就从requiredExtensions中移除，最后如果requiredExtensions是empty就表示都支持。
		for (const auto &extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	// 通常的硬件设备通常只允许为每一个queueFamily创建很少的queue，我们常见的是只需要一个queue。通过在多线程中的创建command buffers，然后在主线程中同时提交到一个queue，这样的开销就非常低了。
	// 这个方法现在找的queueFamily同时进行了判定是否支持present。
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;

		// 查询硬件设配支持多少个QueueFamily，queueFamilies用于存放支持的queueFamilies
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto &queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {// 这是我们找families的条件，（这里的意思是找出queueFlags和VK_QUEUE_GRAPHICS_BIT相等的，queueFlags是VkQueueFlagBits）
				indices.graphicsFamily = i;
			}

			// 支持present判断
			VkBool32 presentSupport = false;
			// device的i的一个队列族是否支持是支持present
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (presentSupport) {
				indices.presentFamily = i;
			}

			if (indices.isComplete()) {
				break;
			}
			i++;
		}

		return indices;
	}

	// 从物理设备的队列族中找到满足条件的队列族，并且返回队列族中满足条件的队列的index。然后用这个index申请使用相应的队列。
	// 这里创建的步骤和前面的比较类似了，就不一一解释了，只写关键步骤注释。
	void createLogicalDevice() {
		// 上一步创建物理设备后我们记录了我们选择的物理设备physicalDevice, 根据physicalDevice来创建逻辑设备
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

		// VkDeviceCreateInfo的pQueueCreateInfos可能不止一个，因此这里要这样改动（如果graphicsFamily和presentFamily不是同一个queue，那就会产生不止一个）
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
		// 调度优先级，范围是0-1
		float queuePriority = 1.0f;	
		// 申请使用的队列
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		// 设备特性，后面将会用到
		VkPhysicalDeviceFeatures deviceFeatures{};

		// 创建逻辑设备。
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		// 队列
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		// 特性
		createInfo.pEnabledFeatures = &deviceFeatures;
		// 扩展
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();
		// 层
		// 这部分逻辑是弃用的了，将会被vkEnumerateDeviceLayerProperties取代
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}

		// 拿到队列的handle
		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
	}

	void createSurface() {
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	// 查询一个物理设备的交换链支持细节，其中formats和presentModes都是数组，里面可能支持了很多种，具体要使用哪一些将会在后面的几个方法中（chooseSwapSurfaceFormat，chooseSwapPresentMode）实现。
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
		SwapChainSupportDetails details;

		// surface capabilities (min/max number of images in swap chain, min/max width and height of images)
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		// Surface formats(pixel format, color space)
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		// Available presentation modes
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	// VkSurfaceFormatKHR选择。
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	// VkPresentModeKHR选择。
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	// 交换内容选择（分辨率）
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != UINT32_MAX) {
			return capabilities.currentExtent;
		}
		else {
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	// 交换链总是和Surface相关联的，surface可以看做我们的显示屏幕，交换链就是图像队列。
	// 用我们当前选中的设备创建SwapChain
	void createSwapChain() {
		SwapChainSupportDetails swapChainSupportDetails = querySwapChainSupport(physicalDevice);

		// 下面三行代码都是根据我们设备支持的交换链详情来选择合适的设置（合适的标准是我们根据需要自己定义的）
		// querySwapChainSupport方法中只是说明了我们的设备支持,findQueueFamilies方法中我们也是这样的，先找出支持的
		// 然后在支持的里面找出满足我们条件的。
		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupportDetails.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupportDetails.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupportDetails.capabilities);

		// 0 is a special value that means that there is no maximum
		uint32_t imageCount = swapChainSupportDetails.capabilities.minImageCount + 1;
		if (swapChainSupportDetails.capabilities.maxImageCount > 0 && imageCount > swapChainSupportDetails.capabilities.maxImageCount) {
			imageCount = swapChainSupportDetails.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		// 非3D模型都使用1
		createInfo.imageArrayLayers = 1;
		// 后处理使用VK_IMAGE_USAGE_TRANSFER_DST_BIT 
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		// 设置QueueFamilies, 这里的队列在createLogicalDevice中已经申请了，这里只是设置一个index而已。
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
		if (indices.graphicsFamily != indices.presentFamily) {
			// 图像可以被多个队列簇访问，不需要明确所有权从属关系。
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			// 同一时间图像只能被一个队列簇占用
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		// 显示内容旋转方向
		createInfo.preTransform = swapChainSupportDetails.capabilities.currentTransform;
		// alpha混合方式
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		// 呈现方式设置
		createInfo.presentMode = presentMode;
		// 裁剪
		createInfo.clipped = VK_TRUE;
		// 未来章节讨论，和窗口大小调整引起的问题有关。
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!");
		}

		// 这些操作是给下一章显示Image做准备的。
		// 获取交换链的图像
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	// 为每一个交换链中的图像创建基本的视图，这些视图在后面的内容中会被作为颜色目标与渲染管线配合使用。
	void createImageViews() {
		// 在渲染管线中VkImageView是用来解释VkImage的信息以及那些可以被访问的，VkImage是不能和渲染管线进行交互的，都是通过VkImageView来操作的。
		swapChainImageViews.resize(swapChainImages.size());
		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create image views!");
			}
		}
	}

	// 整合下面的内容
	// Shader stages: 着色器模块定义了图形管线可编程阶段的功能
	// Fixed - function state : 结构体定义固定管线功能，比如输入装配、光栅化、viewport和color blending
	// Pipeline layout : 管线布局定义uniform 和 push values，被着色器每一次绘制的时候引用
	// Render pass : 渲染通道通过管线阶段引用附件，并定义它的使用方式
	void createGraphicsPipeline() {
		// 首先拿到shader源码
		auto vertShaderCode = readFile("Shaders/vert.spv");
		auto fragShaderCode = readFile("Shaders/frag.spv");

		// 包装shader
		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		// 把shader指定给流水线的管道
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		// shaderCode所在的模块
		vertShaderStageInfo.module = vertShaderModule;
		// 调用函数的名字（入口）
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		// 后面的vertex buffer章节回来完善
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optionaln

		// 暂时不是很理解，后面在来理解，实在不行就查阅其他资料
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		// 视点（viewpoints）和裁剪（这部分原理是自己熟悉的，跳过）
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		// 光栅化
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		// true表示超过远近裁剪面的片元会进行收敛（clamp），而不是丢弃它们。它在特殊的情况下比较有用，像阴影贴图。使用该功能需要得到GPU的支持。
		rasterizer.depthClampEnable = VK_FALSE;
		// true表示几何图元永远不会传递到光栅化阶段。这是基本的禁止任何输出到framebuffer帧缓冲区的方法。
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		// 绘制模式VK_POLYGON_MODE_LINE表示线框模式，下面这个是填充图形模式。
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		// 裁剪面的模式，有剔除前面，剔除背面，和全部。
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		// 正面的定点顺序（顶点顺序好像和正面背面有关，以及法线方向之类的相关）
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		// 光栅化可以通过添加常量或者基于片元的斜率来更改深度值。一些时候对于阴影贴图是有用的，但是我们不会在章节中使用，设置depthBiasEnable为VK_FALSE。
		// 后面会讲到
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

		// 多重采样
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		// VkPipelineColorBlendAttachmentState 包含每一个附加的framebuffer的配置
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		// 颜色写入过滤或者叫色彩通道遮罩，这里是允许写入的颜色
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		// 是否开启颜色混合
		colorBlendAttachment.blendEnable = VK_FALSE;

		// VkPipelineColorBlendStateCreateInfo 是包含了全局混色的设置
		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		// uniform常量创建，暂时不是很明白，后续章节会介绍。
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pushConstantRangeCount = 0;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

		// 创建流水线
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}

		// 完了就卸载。
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device, vertShaderModule, nullptr);
	}

	static std::vector<char> readFile(const std::string &fileName) {
		// ate表示从尾往前读取。
		std::ifstream file(fileName, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}

		// 获得文件大小
		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		// 回到起点, 开始读取数据
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}

	VkShaderModule createShaderModule(const std::vector<char> &code) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		return shaderModule;
	}

	// 渲染通道，在我们完成管线的创建工作之前，我们需要告诉Vulkan渲染时候使用的framebuffer帧缓冲区附件相关信息
	// 我们需要指定多少个颜色和深度缓冲区将会被使用，指定多少个采样器被用到及在整个渲染操作中相关的内容如何处理。所有的这些信息都被封装在一个叫做 render pass 的对象中
	// 一个渲染流程 Render Pass 代表了从各种元数据经过一系列流程最终生成我们需要的一系列图像（不一定是最终呈现在屏幕上的画面）
	// 的过程，而这一系列（可能是颜色、深度模板、适合传送等类型）生成出的画面即为 Attachments，也可被成为渲染目标 Render Targets。
	void createRenderPass() {
		//==============================RenderPass的附件======================================
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainImageFormat;
		// 采样次数
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		// 这两个应用于颜色和深度数据
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		// 这两个应用于模板数据
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		// 这里的知识比较难理解，从CPU linear layout 内存数据 到 GPU optimal layout 就是通过这里设置的
		// 个人理解initialLayout是RendPass前的布局，就是CPU上数据的内存布局；RendPass后就是GPU阶段了，因此
		// finalLayout是GPU上的数据在内存的布局。
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		//==============================RenderPass的附件======================================

		//============================== subpass需要的附件引用======================================
		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		//============================== subpass需要的附件引用======================================

		//============================== subpass======================================
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		//============================== subpass需要的附件引用======================================

		// 创建RenderPass
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}

	// 帧缓冲区可以看作是给render pass写attachment存放的地方。一个 pass 最后往 attachment 里面写东西其实就写在了帧缓冲里面
	// (The attachments specified during render pass creation are bound by wrapping them into a VkFramebuffer object)
	void createFramebuffers() {
		swapChainFramebuffers.resize(swapChainImageViews.size());

		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			VkImageView attachments[] = {
				swapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}

};

int main() {
	HelloTriangleApplication app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}