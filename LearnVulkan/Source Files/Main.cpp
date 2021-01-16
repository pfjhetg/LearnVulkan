#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>

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
// ��ΪvkCreateDebugUtilsMessengerEXT��������һ����չ�еģ�Ҫ������ֻ��ͨ��vkGetInstanceProcAddr�����ҵ�vkCreateDebugUtilsMessengerEXT�����������
// ��ַ��Ȼ����е��ã�����������Ʒ���)
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

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

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

	void initWindow() {
		glfwInit();

		// ��ΪĬ���Ǹ�OpenGL�õģ����ﲻ��Ҫ�����GLFW_NO_API
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// �ر����Ŵ���
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	}

	void initVulkan() {
		createInstance();
		// ����debug
		setupDebugMessenger();
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
		// ���Դ��������������β���Ĵ��뿪ʼ��ǰ�������
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
		// �����µĻ�ȡ��չ�ķ�ʽ���������ɵ�
		auto extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		if (enableValidationLayers) {// ���Ҫ���õĲ�
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
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

	// ������ʽ��ʹ, �����createInstance�Ƚ����ƣ���˿�����ͬ������ⲽ����������������
	void setupDebugMessenger() {
		if (!enableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallBack;
		// ��ѡ�ģ��������ǲ���Ҫ
		createInfo.pUserData = nullptr;

		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger!");
		}
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