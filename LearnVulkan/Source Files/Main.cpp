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
// ����չһ������֤��Ҳ��ͨ��ָ�����������������Vulkan�ṩ�ı�׼����֤�㱻��װ��һ������ΪVK_LAYER_KHRONOS_validation�Ĳ��У��������Ǿ��ǿ���vulkan�ṩ����֤�㡣
const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// ��instance��Ѱ��vkCreateDebugUtilsMessengerEXT�����ĵ�ַ����������˶�Ӧ�Ĳ㣨VK_EXT_DEBUG_UTILS_EXTENSION_NAME������ô�ͻ��ҵ���Ӧ���������߷���nullptr��
// ��ΪvkCreateDebugUtilsMessengerEXT��������һ����չ�еģ�Ҫ������ֻ��ͨ��vkGetInstanceProcAddr���ҵ�vkCreateDebugUtilsMessengerEXT�������
// Ȼ����е��ã�����������Ʒ���)
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

// ���� ԭ��ͬ������
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

// ����ṹ����Ϊ�˸��������װ�ģ���optional������һ�ֺܺõ��ж�û�и�ֵ�ķ�ʽ��
// ������һ���Ƿ�֧��present���жϡ�
struct QueueFamilyIndices {
	std::optional<uint32_t>graphicsFamily;
	// ��ʾ֧��surface��present
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

// ����ṹ���QeueuFamilyIndices����ԭ�����ƣ������жϽ�������Ҫ���������ԡ�
struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

// ��������Ҫ֧�ֵ���չ
const std::vector<const char *> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

class HelloTriangleApplication {
public:
	void run() {
		// ��ʼ��GLFW
		initWindow();
		// ��ʼ��Vulkan
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
	// ����WSI (Window System Integration) extensions.��VK_KHR_surface��������glfwGetRequiredInstanceExtensions�С����ʵ���Ǻ�ƽ̨�޹ص�
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

		// ��ΪĬ���Ǹ�OpenGL�õģ����ﲻ��Ҫ�����GLFW_NO_API
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// �ر����Ŵ���
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	}

	void initVulkan() {
		// ����ʵ�� ���벽��
		createInstance();
		// ����debug ��ѡ
		setupDebugMessenger();
		// ������������Ϊ��һ����Ӱ��pickPhysicalDevice��(�̳�����һ�����ڴ����߼��豸�����) ���벽��
		createSurface();
		// ѡ�������豸 ���벽��
		pickPhysicalDevice();
		// �����߼��豸 ���벽��
		createLogicalDevice();
		// ����ǰ���һЩ׼����Ȼ�������ﴴ�������� ���벽��
		createSwapChain();
		// ���벽��
		createImageViews();
		// ���벽��
		createRenderPass();
		// ���벽��
		createGraphicsPipeline();
		// ���벽��
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
		// ���ʵ���������ʱ����Ҫ�Լ�����
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);

		glfwTerminate();
	}

	// ����Vulkanʵ������������Ӧ�ú�Vulkan��
	void createInstance() {
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available!");
		}
		VkInstanceCreateInfo createInfo{};
		// ���Դ��������������β���Ĵ��뿪ʼ��ǰ������⣬���Ȳ���API����Ҫ�Ǽ���������Ȼ��һһ������䡣
		// 1.Ҫ����һ��ʵ������Ҫָ��һЩ�в���
		// 2.�����������֤�㣬�Ϳ����������ӵ���֤��
		// 3.��չ�����Ͷ�Ӧ����չ������ͨ�����沿��ר�ŵķ�����ȡ��vulkan�ܶ�ط�����ͨ���������Ʒ�ʽ��ȡ�ģ�һ��Ҫ��Ϥ���ַ�ʽ��
		// 4.pApplicationInfo������Ҫһ���µĶ�����˾����˴�����ǰ�沿�ֵ�VkApplicationInfo

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
			// ���µĻ�ȡ��չ�ķ�ʽ���������ɵ�
		auto extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		// ���debugCreateInfo��������vkCreateInstance��vkDestroyInstance֮����Եġ�
		// ǰ���½ڵ�CreateDebugUtilsMessengerEXT��DestroyDebugUtilsMessengerEXT�ֱ���vkCreateInstanceǰ��vkDestroyInstance��
		// ����޷�����vkCreateInstance��vkDestroyInstance֮����κ����⡣
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if (enableValidationLayers) {// ���Ҫ���õĲ�
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			// ������֪ʶ���Բο�https://github.com/KhronosGroup/Vulkan-Docs/blob/master/appendices/VK_EXT_debug_utils.txt#L120�е�
			// 84�и����Ĵ��룬ע���ر�˵���������ķŵ�callbackֻ����vkCreateInstance��vkDestroyInstance֮�䴥��
			// �������������ˣ�����ר�ŵ���vkCreateDebugUtilsMessengerEXT��vkDestroyDebugUtilsMessengerEXT�ˣ��ڲ������˻��Զ����ã��������������Ǹ���createInfo�ġ�
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		// ��󴴽�ʵ��
		// ��һ�������������������õ��Ĵ�����Ϣ���ڶ����ǻص������ﲻ��ע���գ����������Ǵ������Handle
		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}

	// ������ʽ��ʹ(ֱ�룬�����̬�Ϳ���), �����createInstance�Ƚ����ƣ���˿�����ͬ������ⲽ����������������
	void setupDebugMessenger() {
		if (!enableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		/*createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallBack;*/
		// ���createInfo����
		populateDebugMessengerCreateInfo(createInfo);
		// ��ѡ�ģ��������ǲ���Ҫ
		createInfo.pUserData = nullptr;

		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	// �ⲿ�ִ�����������Ϊ��������vkCreateInstance��vkDestroyInstance���������ڵ�debug��
	// �������& ��ʾ���ã����þ���ĳһ������Ŀ�꣩��һ�������������õĲ�����Ա���ֱ�Ӳ�����ȫһ����������c++֪ʶ��Ҫ��ʱ��ϰ��
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallBack;
	}

	bool checkValidationLayerSupport() {
		uint32_t layerCount;
		// �������Ϊnullptrʱ���ǻ�ȡ�������
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		// availableLayers�������ϵͳ֧�ֵĲ�
		std::vector<VkLayerProperties> availableLayers(layerCount);
		// ���������Ϊnullptrʱ�򣬱�ʾ�鼰�㣬Ȼ�����θ�ֵ��
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		//������ǿ�������֤���Ƿ���ϵͳ֧�ֵ����п��õĲ���
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

	// ��װ��һ�»�ȡ��չ�ķ�����glfw��������չ��һֱ��Ҫ�ģ�debug messenger��չ�Ǹ����Ƿ�������֤��������ġ�               
	// ǰ��һ����ֱ�Ӱ�glfw��������չ����createInfo��Ҫ����Ϣ�ˣ�����������һ����ѡ��debug messenger��չ������б�Ҫ���·�װһ�¡�
	std::vector<const char *> getRequiredExtensions() {
		uint32_t glfwExtensionsCount = 0;
		const char **glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
		// �ӻ�glfwExtensions��0��Ԫ��ʼ����glfwExtensionsCount��Ԫ��ֹ��Ԫ������ʼ��Vector������std::vector�ĳ�ʼ����ʽ�������˿���Google��
		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionsCount);

		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		return extensions;
	}

	// The VKAPI_ATTR and VKAPI_CALL ensure that the function has the right signature for Vulkan to call it
	// ��һ����������Ϣ�����س̶�
	// �ڶ����������¼�����
	// ��������������ϸ����Ϣ
	// ���ĸ����������û������Զ������ݵ�
	// If the callback returns true, then the call is aborted with the VK_ERROR_VALIDATION_FAILED_EXT error.
	// ����ֻ�ǲ�����֤�㣬���Է���VK_FALAE����
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallBack(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
		return VK_FALSE;
	}

	void pickPhysicalDevice() {
		uint32_t deviceCount = 0;
		// �����������֧�ֵ�Ӳ���豸
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		if (deviceCount == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		// ����ֻ��һ���̳̣��������ദ������ҵ�һ���ʺϵľͿ��ԣ�Ҳ���Ը��ݽ̳�˵�ð��������Լ�����һ���÷�����������ȡ��
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

	// �ж�һ��Ӳ���豸�Ƿ��ʺ�(�����ж϶����Ƿ�֧�֣�Ȼ���ж��Ƿ�֧��ָ������չ������жϽ�����֧�����)
	bool isDeviceSuitable(VkPhysicalDevice device) {
		// findQueueFamilies�еĲ����ǻ����豸�еĶ����壨queueFamilyes����������е�
		QueueFamilyIndices indices = findQueueFamilies(device);
		bool extensionsSupported = checkDeviceExtensionSupport(device);
		// �ж�details of swap chain support
		bool swapChainAdequate = false;
		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupportDetails = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupportDetails.formats.empty() && !swapChainSupportDetails.presentModes.empty();
		}

		return indices.isComplete() && extensionsSupported && swapChainAdequate;
	}

	// �ж�Ӳ���Ƿ�֧��deviceExtensions�е���չ
	bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
		// ȡ��Ӳ��֧�ֵ���չ
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		// ���ǵĳ�������Ҫ�õ�����չ
		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
		// ����֧�ֵ���չ����������ĳһ������������Ҫ����չ�У��ʹ�requiredExtensions���Ƴ���������requiredExtensions��empty�ͱ�ʾ��֧�֡�
		for (const auto &extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	// ͨ����Ӳ���豸ͨ��ֻ����Ϊÿһ��queueFamily�������ٵ�queue�����ǳ�������ֻ��Ҫһ��queue��ͨ���ڶ��߳��еĴ���command buffers��Ȼ�������߳���ͬʱ�ύ��һ��queue�������Ŀ����ͷǳ����ˡ�
	// ������������ҵ�queueFamilyͬʱ�������ж��Ƿ�֧��present��
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;

		// ��ѯӲ������֧�ֶ��ٸ�QueueFamily��queueFamilies���ڴ��֧�ֵ�queueFamilies
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto &queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {// ����������families�����������������˼���ҳ�queueFlags��VK_QUEUE_GRAPHICS_BIT��ȵģ�queueFlags��VkQueueFlagBits��
				indices.graphicsFamily = i;
			}

			// ֧��present�ж�
			VkBool32 presentSupport = false;
			// device��i��һ���������Ƿ�֧����֧��present
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

	// �������豸�Ķ��������ҵ����������Ķ����壬���ҷ��ض����������������Ķ��е�index��Ȼ�������index����ʹ����Ӧ�Ķ��С�
	// ���ﴴ���Ĳ����ǰ��ıȽ������ˣ��Ͳ�һһ�����ˣ�ֻд�ؼ�����ע�͡�
	void createLogicalDevice() {
		// ��һ�����������豸�����Ǽ�¼������ѡ��������豸physicalDevice, ����physicalDevice�������߼��豸
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

		// VkDeviceCreateInfo��pQueueCreateInfos���ܲ�ֹһ�����������Ҫ�����Ķ������graphicsFamily��presentFamily����ͬһ��queue���Ǿͻ������ֹһ����
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
		// �������ȼ�����Χ��0-1
		float queuePriority = 1.0f;	
		// ����ʹ�õĶ���
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		// �豸���ԣ����潫���õ�
		VkPhysicalDeviceFeatures deviceFeatures{};

		// �����߼��豸��
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		// ����
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		// ����
		createInfo.pEnabledFeatures = &deviceFeatures;
		// ��չ
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();
		// ��
		// �ⲿ���߼������õ��ˣ����ᱻvkEnumerateDeviceLayerPropertiesȡ��
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

		// �õ����е�handle
		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
	}

	void createSurface() {
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	// ��ѯһ�������豸�Ľ�����֧��ϸ�ڣ�����formats��presentModes�������飬�������֧���˺ܶ��֣�����Ҫʹ����һЩ�����ں���ļ��������У�chooseSwapSurfaceFormat��chooseSwapPresentMode��ʵ�֡�
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

	// VkSurfaceFormatKHRѡ��
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	// VkPresentModeKHRѡ��
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	// ��������ѡ�񣨷ֱ��ʣ�
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

	// ���������Ǻ�Surface������ģ�surface���Կ������ǵ���ʾ��Ļ������������ͼ����С�
	// �����ǵ�ǰѡ�е��豸����SwapChain
	void createSwapChain() {
		SwapChainSupportDetails swapChainSupportDetails = querySwapChainSupport(physicalDevice);

		// �������д��붼�Ǹ��������豸֧�ֵĽ�����������ѡ����ʵ����ã����ʵı�׼�����Ǹ�����Ҫ�Լ�����ģ�
		// querySwapChainSupport������ֻ��˵�������ǵ��豸֧��,findQueueFamilies����������Ҳ�������ģ����ҳ�֧�ֵ�
		// Ȼ����֧�ֵ������ҳ��������������ġ�
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
		// ��3Dģ�Ͷ�ʹ��1
		createInfo.imageArrayLayers = 1;
		// ����ʹ��VK_IMAGE_USAGE_TRANSFER_DST_BIT 
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		// ����QueueFamilies, ����Ķ�����createLogicalDevice���Ѿ������ˣ�����ֻ������һ��index���ѡ�
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
		if (indices.graphicsFamily != indices.presentFamily) {
			// ͼ����Ա�������дط��ʣ�����Ҫ��ȷ����Ȩ������ϵ��
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			// ͬһʱ��ͼ��ֻ�ܱ�һ�����д�ռ��
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		// ��ʾ������ת����
		createInfo.preTransform = swapChainSupportDetails.capabilities.currentTransform;
		// alpha��Ϸ�ʽ
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		// ���ַ�ʽ����
		createInfo.presentMode = presentMode;
		// �ü�
		createInfo.clipped = VK_TRUE;
		// δ���½����ۣ��ʹ��ڴ�С��������������йء�
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!");
		}

		// ��Щ�����Ǹ���һ����ʾImage��׼���ġ�
		// ��ȡ��������ͼ��
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	// Ϊÿһ���������е�ͼ�񴴽���������ͼ����Щ��ͼ�ں���������лᱻ��Ϊ��ɫĿ������Ⱦ�������ʹ�á�
	void createImageViews() {
		// ����Ⱦ������VkImageView����������VkImage����Ϣ�Լ���Щ���Ա����ʵģ�VkImage�ǲ��ܺ���Ⱦ���߽��н����ģ�����ͨ��VkImageView�������ġ�
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

	// �������������
	// Shader stages: ��ɫ��ģ�鶨����ͼ�ι��߿ɱ�̽׶εĹ���
	// Fixed - function state : �ṹ�嶨��̶����߹��ܣ���������װ�䡢��դ����viewport��color blending
	// Pipeline layout : ���߲��ֶ���uniform �� push values������ɫ��ÿһ�λ��Ƶ�ʱ������
	// Render pass : ��Ⱦͨ��ͨ�����߽׶����ø���������������ʹ�÷�ʽ
	void createGraphicsPipeline() {
		// �����õ�shaderԴ��
		auto vertShaderCode = readFile("Shaders/vert.spv");
		auto fragShaderCode = readFile("Shaders/frag.spv");

		// ��װshader
		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		// ��shaderָ������ˮ�ߵĹܵ�
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		// shaderCode���ڵ�ģ��
		vertShaderStageInfo.module = vertShaderModule;
		// ���ú��������֣���ڣ�
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		// �����vertex buffer�½ڻ�������
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optionaln

		// ��ʱ���Ǻ���⣬����������⣬ʵ�ڲ��оͲ�����������
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		// �ӵ㣨viewpoints���Ͳü����ⲿ��ԭ�����Լ���Ϥ�ģ�������
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

		// ��դ��
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		// true��ʾ����Զ���ü����ƬԪ�����������clamp���������Ƕ������ǡ��������������±Ƚ����ã�����Ӱ��ͼ��ʹ�øù�����Ҫ�õ�GPU��֧�֡�
		rasterizer.depthClampEnable = VK_FALSE;
		// true��ʾ����ͼԪ��Զ���ᴫ�ݵ���դ���׶Ρ����ǻ����Ľ�ֹ�κ������framebuffer֡�������ķ�����
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		// ����ģʽVK_POLYGON_MODE_LINE��ʾ�߿�ģʽ��������������ͼ��ģʽ��
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		// �ü����ģʽ�����޳�ǰ�棬�޳����棬��ȫ����
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		// ����Ķ���˳�򣨶���˳���������汳���йأ��Լ����߷���֮�����أ�
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		// ��դ������ͨ����ӳ������߻���ƬԪ��б�����������ֵ��һЩʱ�������Ӱ��ͼ�����õģ��������ǲ������½���ʹ�ã�����depthBiasEnableΪVK_FALSE��
		// ����ὲ��
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

		// ���ز���
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		// VkPipelineColorBlendAttachmentState ����ÿһ�����ӵ�framebuffer������
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		// ��ɫд����˻��߽�ɫ��ͨ�����֣�����������д�����ɫ
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		// �Ƿ�����ɫ���
		colorBlendAttachment.blendEnable = VK_FALSE;

		// VkPipelineColorBlendStateCreateInfo �ǰ�����ȫ�ֻ�ɫ������
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

		// uniform������������ʱ���Ǻ����ף������½ڻ���ܡ�
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pushConstantRangeCount = 0;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

		// ������ˮ��
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

		// ���˾�ж�ء�
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device, vertShaderModule, nullptr);
	}

	static std::vector<char> readFile(const std::string &fileName) {
		// ate��ʾ��β��ǰ��ȡ��
		std::ifstream file(fileName, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}

		// ����ļ���С
		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		// �ص����, ��ʼ��ȡ����
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

	// ��Ⱦͨ������������ɹ��ߵĴ�������֮ǰ��������Ҫ����Vulkan��Ⱦʱ��ʹ�õ�framebuffer֡���������������Ϣ
	// ������Ҫָ�����ٸ���ɫ����Ȼ��������ᱻʹ�ã�ָ�����ٸ����������õ�����������Ⱦ��������ص�������δ������е���Щ��Ϣ������װ��һ������ render pass �Ķ�����
	// һ����Ⱦ���� Render Pass �����˴Ӹ���Ԫ���ݾ���һϵ��������������������Ҫ��һϵ��ͼ�񣨲�һ�������ճ�������Ļ�ϵĻ��棩
	// �Ĺ��̣�����һϵ�У���������ɫ�����ģ�塢�ʺϴ��͵����ͣ����ɳ��Ļ��漴Ϊ Attachments��Ҳ�ɱ���Ϊ��ȾĿ�� Render Targets��
	void createRenderPass() {
		//==============================RenderPass�ĸ���======================================
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainImageFormat;
		// ��������
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		// ������Ӧ������ɫ���������
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		// ������Ӧ����ģ������
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		// �����֪ʶ�Ƚ�����⣬��CPU linear layout �ڴ����� �� GPU optimal layout ����ͨ���������õ�
		// �������initialLayout��RendPassǰ�Ĳ��֣�����CPU�����ݵ��ڴ沼�֣�RendPass�����GPU�׶��ˣ����
		// finalLayout��GPU�ϵ��������ڴ�Ĳ��֡�
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		//==============================RenderPass�ĸ���======================================

		//============================== subpass��Ҫ�ĸ�������======================================
		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		//============================== subpass��Ҫ�ĸ�������======================================

		//============================== subpass======================================
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		//============================== subpass��Ҫ�ĸ�������======================================

		// ����RenderPass
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

	// ֡���������Կ����Ǹ�render passдattachment��ŵĵط���һ�� pass ����� attachment ����д������ʵ��д����֡��������
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