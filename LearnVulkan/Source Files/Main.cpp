#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <optional>
#include <set>

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
		// ������������Ϊ��һ����Ӱ��pickPhysicalDevice��
		createSurface();
		// ѡ�������豸 ���벽��
		pickPhysicalDevice();
		// �����߼��豸 ���벽��
		createLogicalDevice();
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
		vkDestroyDevice(device, nullptr);
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

	// �ж�һ��Ӳ���豸�Ƿ��ʺ�
	bool isDeviceSuitable(VkPhysicalDevice device) {
		// findQueueFamilies�еĲ����ǻ����豸�еĶ����壨queueFamilyes����������е�
		QueueFamilyIndices indices = findQueueFamilies(device);
		return indices.isComplete();
	}

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

	// ��ǰ��Ӳ���豸ͨ��ֻ����Ϊÿһ��queueFamily�������ٵ�queue�����Ǹ���������ֻ��Ҫһ��queue��ͨ���ڶ��߳��еĴ���command buffers��Ȼ�������߳���ͬʱ�ύ�������Ŀ����ͷǳ����ˡ�
	// (findQueueFamilies����ע���е����������������)
	// ���ﴴ���Ĳ����ǰ��ıȽ������ˣ��Ͳ�һһ�����ˣ�ֻд�ؼ�����ע�͡�
	void createLogicalDevice() {
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

		// VkDeviceCreateInfo��pQueueCreateInfos���ܲ�ֹһ���ˣ��������Ҫ�����Ķ������graphicsFamily��presentFamily����ͬһ��queue���Ǿͻ������ֹһ����
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
		// �������ȼ�����Χ��0-1
		float queuePriority = 1.0f;
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
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = 0;

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