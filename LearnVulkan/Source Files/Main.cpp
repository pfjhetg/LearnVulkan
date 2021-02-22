#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <array>
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

const int MAX_FRAMES_IN_FLIGHT = 2;

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
		// 
		mainLoop();
		// 
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
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	size_t currentFrame = 0;
	bool framebufferResized = false;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	void initWindow() {
		glfwInit();

		// 因为默认是给OpenGL用的，这里不需要因此用GLFW_NO_API
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// 关闭缩放窗口
		//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}

	void initVulkan() {
		// 创建实例 必须步骤
		createInstance();
		// 设置debug 可选
		setupDebugMessenger();
		// 放在这里是因为这一步会影响pickPhysicalDevice。(教程中这一章节是放在创建逻辑设备后面的) 必须步骤
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
		// 必须步骤
		createCommandPool();
		// 优化步骤
		createVertexBuffer();
		createIndexBuffer();
		// 必须步骤
		createCommandBuffers();
		// 优化步骤
		createSyncObjects();
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			drawFrame();
		}
		vkDeviceWaitIdle(device);
	}

	void drawFrame() {
		// 首次执行的时候，inFlightFences[currentFrame]默认是signaled的（创建的时候初始化成了success）。
		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		// Retrieve the index of the next available presentable image
		// 当presentation engine用完了这个vkAcquireNextImageKHR的image后，会发出imageAvailableSemaphores[currentFrame]信号。
		VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}
		// 首次不会执行这里。
		// 这里的imagesInFlight相关代码是用来防止MAX_FRAMES_IN_FLIGHT和swapchain的image数量不一致的情况的。如果不一致有可能取到的Image是InFlight中的。
		// 比如说MAX_FRAMES_IN_FLIGHT和是3，swapchain数量是2，那么MAX_FRAMES_IN_FLIGHT=3时候取到的imageIndex就是0，这个时候有可能这个Image还在InFlight中。
		if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
			// 这里的vkWaitForFences和第一个vkWaitForFences等待的是一个东西，不会在逻辑中判断下一个信号发出，判断的是同一个信号。
			vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
		}
		imagesInFlight[imageIndex] = inFlightFences[currentFrame];

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		// 这是开始执行命令前的等待信号
		// 这里指定imageAvailableSemaphores的传入队列。
		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		// VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT是指定混合后管道的阶段，从管道输出最终颜色值
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		// 这两句代码表示命令开始前的等待：在waiStages所在的阶段中等待在waitSemaphores中对应的VkSemaphore。
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		// initVulkan中所有的设置都会在commandBuffer中生效。
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

		// 这是命令执行完成的发出信号
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		// 当此批处理的命令缓冲区完成执行时，将发出信号
		submitInfo.pSignalSemaphores = signalSemaphores;
		// unsignaled 
		vkResetFences(device, 1, &inFlightFences[currentFrame]);

		// 这里的inFlightFences[currentFrame]会在命令缓冲区完成执行发出信号，在vkWaitForFences的地方就可以被知晓已经收到信号，可以继续程序接下来的操作。
		// 提交执行渲染任务
		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		// 呈现渲染数据的时候有可能vkQueueSubmit没有执行完，因此我们会设置pWaitSemaphores。
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = &imageIndex;

		result = vkQueuePresentKHR(presentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
			framebufferResized = false;
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

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
			// 84行附近的代码，注释特别说明了这样的callback只会在vkCreateInstance和vkDestroyInstance之间触发
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

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
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

	// surface是一个extensions，这里创建一个主要是看我们的平台是否支持surface这个扩展。
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
		// 创建
		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!");
		}

		// 获取与交换链中关联的可呈现图像的数组
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

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

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
		// Attachments 类似OPENGL的Render Target
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

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		// 创建RenderPass的一些相关信息（这里好像并没有真的创建，只是一些相关设置。createCommandBuffers中才是真的创建）
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}

	// 帧缓冲区可以看作是给render pass写attachment存放的地方。一个 pass 最后往 attachment 里面写东西其实就写在了帧缓冲里面
	// (The attachments specified during render pass creation are bound by wrapping them into a VkFramebuffer object)
	// 这个可以看作是最终存放流水线产出的东西的地方。
	void createFramebuffers() {
		// image在流水线中要用ImageView形式表示；每一个ImageView需要一个对应的FrameBuffer用于attachment操作。
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

	void createCommandPool() {
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
		poolInfo.flags = 0;// Optional

		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
	}

	// 用于记录行为命令，如果是绘制图形的命令，需要绑定framebuffer因为最终绘制的内容是存放在framebuffer中的。
	void createCommandBuffers() {
		// 交换链中每一个对象都需要分配一个commandBuffer，表示渲染这一个帧缓冲的命令。
		commandBuffers.resize(swapChainFramebuffers.size());

		// 初始化commandBuff，注意初始化多个也是通过一个allocate。
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

		if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}

		// Starting command buffer recording
		// 录制命令从vkBeginCommandBuffer开始，以vkEndCommandBuffer结束。
		for (size_t i = 0; i < commandBuffers.size(); i++) {
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0; // Optional
			beginInfo.pInheritanceInfo = nullptr; // Optional

			if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
				throw std::runtime_error("failed to begin recording command buffer!");
			}

			// Starting a render pass
			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			// The first parameters are the render pass itself and the attachments to bind
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = swapChainFramebuffers[i];

			// 渲染区域和大小,renderArea是一个VkRect2D，offset表示xy，extent表示with，height。
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = swapChainExtent;

			VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			// Basic drawing commands 用commangbuffer来提交这次renderPassInfo的渲染。
			// commandBuffers[i]是用于记录命令的
			// renderPassInfo提供渲染的一些细节
			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			// bind the graphics pipeline:
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			// 把顶点缓冲绑定到流水线上
			VkBuffer vertexBuffers[] = { vertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);

			// draw
			//vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);
			vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

			// end render pass 
			vkCmdEndRenderPass(commandBuffers[i]);

			if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}
		}
	}

	// 创建buffer步骤：
	// 构建VkBufferCreateInfo信息，创建VkBuffer。
	// 根据VkBuffer获取需要的内存类型VkMemoryRequirements
	// 根据VkMemoryRequirements设置VkMemoryAllocateInfo信息，VkMemoryAllocateInfo信息还需要一些其他设置
	// 根据VkMemoryAllocateInfo信息来分配内存得到VkDeviceMemory。
	// 最后要把VkBuffer和VkDeviceMemory绑定，表面VkDeviceMemory是分配给VkBuffer的内存。
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		// 这个缓冲给谁用
		bufferInfo.usage = usage;
		// 这个缓冲是否分享给多个 queue
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create vertex buffer!");
		}

		// 根据vertexBuffer获取缓冲所需内存类型
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		// 内存的一些设置（大小，种类等）
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT对 CPU 可见, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT memcpy 之后不需要显式调用 flush 和 invalidate 系列指令
		//保证客户端有能力去映射这一片显存中分配的buffer并且进行修改(VkMemoryPropertyFlags中的枚举)
		// memoryTypeIndex就是内存种类
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		// 分配内存
		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate vertex buffer memory!");
		}

		// 将分配的内存和vertexBuffer缓冲绑定
		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

	// 同步对象创建
	void createSyncObjects() {
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		// fence默认状态设置成VK_SUCCESS
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create synchronization objects for a frame!");
			}
			VkResult vkResult = vkGetFenceStatus(device, inFlightFences[i]);
			std::cout << vkResult;
		}
	}


	void cleanup() {
		cleanupSwapChain();
		vkDestroyBuffer(device, indexBuffer, nullptr);
		vkFreeMemory(device, indexBufferMemory, nullptr);

		vkDestroyBuffer(device, vertexBuffer, nullptr);
		vkFreeMemory(device, vertexBufferMemory, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}

		vkDestroyCommandPool(device, commandPool, nullptr);

		vkDestroyDevice(device, nullptr);

		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);

		glfwTerminate();
	}


	void cleanupSwapChain() {
		for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
			vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
		}

		vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);

		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			vkDestroyImageView(device, swapChainImageViews[i], nullptr);
		}

		vkDestroySwapchainKHR(device, swapChain, nullptr);
	}

	void recreateSwapChain() {
		// 处理最小化
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}
		vkDeviceWaitIdle(device);

		cleanupSwapChain();

		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandBuffers();
	}

	struct Vertex {
		glm::vec2 pos;
		glm::vec3 color;

		// 如何解释顶点属性并绑定到着色器中, 针对逐顶点数据，我们所有的顶点数据发送到GPU后都会打包到一个数组上, 这个数组可以叫bindings。
		// binding就是这个数组开始读取的index。（逐实例有点类似GPU 实例化，需要深入了解可以Google）
		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			// 将会绑定哪个顶点缓冲编号
			bindingDescription.binding = 0;
			// 步长，数据跨度
			bindingDescription.stride = sizeof(Vertex);
			// 扫描数据方式，这里是逐顶点。(另一种方式是逐实例，通常用于GPU实例化)
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		// 如何处理顶点的输入
		// 描述的是怎么样从VkVertexInputBindingDescription后的绑定好的原始数据块中提取出一个个顶点属性。
		// 这里有2个VkVertexInputAttributeDescription分别用来描述pos和color。
		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

			// 步长和偏移量和OPENGL中读取顶点数据一样，步长用来分开每组顶点数据，偏移用来分割每组顶点数据里面的不同数据（颜色，坐标等）。

			// 将会绑定哪个顶点缓冲编号
			attributeDescriptions[0].binding = 0;
			// 对应着Vertex Shader中的槽位（Slot)
			attributeDescriptions[0].location = 0;
			// 该属性隐式体现了数据跨度stride
			attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			// 属性的偏移量
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			// 对应着Vertex Shader中的槽位（Slot）
			attributeDescriptions[1].location = 1;
			// 该属性隐式体现了数据跨度stride
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			// 属性的偏移量
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			return attributeDescriptions;
		}
	};

	const std::vector<Vertex> vertices = {
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};

	const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
	};

	// 暂存区只能被cpu访问，GPU从实际的vertexBuffer区访问比从暂存区快，因此会有下面的优化。
	// 第一次加顶点载数据到CPU和GPU共享访问的暂存区，然后复制到GPU独占的内存区域的vertexBuffer,后续使用直接访问更快。 
	void createVertexBuffer() {
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
		// 暂存缓冲区
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
		// 填充顶点缓冲
		void* data;
		// 使指针指向具体的内存位置, 用data获取到位置
		//// vkMapMemory作用是标记（映射）这个内存区域可以被CPU访问。这一步完成了才可以进行赋值。
		// 将顶点数组 vertices.data() 内存拷贝到这个指针指向映射内存中
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		// 复制顶点数据到暂存区
		memcpy(data, vertices.data(), (size_t)bufferSize);
		// 取消内存的映射
		vkUnmapMemory(device, stagingBufferMemory);

		// 
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
		copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	// 和createVertexBuffer类似的操作。
	void createIndexBuffer() {
		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

		copyBuffer(stagingBuffer, indexBuffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	// 寻找指定的内存类型，返回index
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		// 遍历有效的内存类型
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		// typeFilter参数将以位的形式代表适合的内存类型。这意味着通过简单的迭代内存属性集合，并根据需要的类型与每个内存属性的类型进行AND操作，判断是否为1
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			// memoryTypes : 内存种类
			//  Buffer 需求和设备能满足的需求都获取到时
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
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