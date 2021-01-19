#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <optional>

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
struct QueueFamilyIndices {
	std::optional<uint32_t>graphicsFamily;

	bool isComplete() {
		return graphicsFamily.has_value();
	}
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

	void initWindow() {
		glfwInit();

		// 因为默认是给OpenGL用的，这里不需要因此用GLFW_NO_API
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// 关闭缩放窗口
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	}

	void initVulkan() {
		createInstance();
		// 设置debug
		setupDebugMessenger();
		pickPhysicalDevice();
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}
	}

	void cleanup() {
		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}
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

	// 判断一个硬件设备是否适合
	bool isDeviceSuitable(VkPhysicalDevice device) {
		// 这部分注释掉的是前面讲的一些比较基础的属性和特性,
		// 这部分是基于查找整个硬件设备的信息
		// 整个硬件设备的属性
	/*	VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		// 整个硬件设备支持的特性
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
			deviceFeatures.geometryShader;*/
		// findQueueFamilies中的查找是基于设备中的队列族（queueFamilyes）来分类进行的
		QueueFamilyIndices indices = findQueueFamilies(device);
		return indices.isComplete();
	}

	// 这部分才是本章节重点,找到满足条件的Family的index
	// Queue Families是一个有相同功能的Queues的集合，（一般的显卡是三个，第一个支持全部VkQueueFlagBits，第二个支持后面三个VkQueueFlagBits，第三个支持最后两个VkQueueFlagBits）
	// 一般情况下我们都选一个，一般渲染也只用一个，如果要同时提交多个可以合并了一起提交，个人理解有点类似drawcall，具体在下一节学了后在一起理解。
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

			if (indices.isComplete()) {
				break;
			}
			i++;
		}

		return indices;
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