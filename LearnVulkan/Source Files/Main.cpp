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
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}
	}

	void cleanup() {
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

		uint32_t glfwExtensionCount = 0;
		const char **glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		createInfo.enabledExtensionCount = glfwExtensionCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;

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