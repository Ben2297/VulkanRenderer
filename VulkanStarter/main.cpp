#define GLFW_INCLUDE_VULKAN //automatically loads vulkan header with GLFW
#include <GLFW/glfw3.h> //includes GLFW definitions

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp> //includes the GLM library
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <iostream> //for reporting and propagating errors
#include <fstream> //for file input/output
#include <stdexcept> //for reporting and propagating errors
#include <algorithm>
#include <vector> //for the use of vector data structures
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <optional>
#include <set>
#include <array>
#include <chrono>
#include <unordered_map>

#include "diredge.h"

const int WIDTH = 1000; //constant value for width of window
const int HEIGHT = 800; //constant value for height of window

const std::string MODEL_PATH = "models/sphere.obj";
const std::string TEXTURE_PATH = "textures/furmap.gif";
const std::string FIN_TEXTURE_PATH = "textures/fin.png";

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = { //includes useful standard validation
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = { //declares a list of required device extensions
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG //checks whether program is being compiled in debug mode or not (NDEBUG = not debug mode)
const bool enableValidationLayers = false; //sets boolean to disable validation layers
#else //if in debug mode
const bool enableValidationLayers = true; //sets boolean to enable validation layers
#endif //ends if statement

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"); //gets address of the create messenger function
	if (func != nullptr) { //if the function was loaded
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger); //calls the function
	}
	else { //if the function was not loaded
		return VK_ERROR_EXTENSION_NOT_PRESENT; //returns an error
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"); //gets the address of the destroy messenger function
	if (func != nullptr) { //if function was loaded
		func(instance, debugMessenger, pAllocator); //calls the function
	}
}

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily; //optional value to store the graphics queue family
	std::optional<uint32_t> presentFamily; //optional value to store the presentation queue family

	bool isComplete() { //function to check for the existence of both graphics and presentation queue families 
		return graphicsFamily.has_value() && presentFamily.has_value(); //returns true if both queue families exist
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities; //struct for storing information about the basic surface capabilities
	std::vector<VkSurfaceFormatKHR> formats; //list of structs for storing the surface formats
	std::vector<VkPresentModeKHR> presentModes; //list of structs for storing available presentation modes
};

struct Vertex {
	glm::vec3 pos; //vertex position
	glm::vec3 color; //vertex color
	glm::vec2 texCoord; //texture coordinates
	glm::vec3 normal; //vertex normal

	static VkVertexInputBindingDescription getBindingDescription() { //describes at which rate to load data from memory throughout vertices
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, normal);

		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(4) float renderTex;
	alignas(16) glm::mat4 defaultModel;
};

struct ShadowBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct LightingConstants {
	alignas(16) glm::vec3 lightPosition;
	alignas(16) glm::vec3 lightAmbient;
	alignas(16) glm::vec3 lightDiffuse;
	alignas(16) glm::vec3 lightSpecular;
	alignas(4) float lightSpecularExponent;
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

class HelloTriangleApplication {
public:
	void run() {
		initWindow(); //calls the function to initialise the window
		initVulkan(); //calls the function to initialise vulkan
		mainLoop(); //calls main loop function
		cleanup(); //calls cleanup function
	}

private:
	GLFWwindow* window;	//pointer for the glfw window

	VkInstance instance; //class member to hold handle to instance
	VkDebugUtilsMessengerEXT debugMessenger; //creates a debug messenger object
	VkSurfaceKHR surface; //creates a surface object

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; //creates the physical device object and sets to null
	VkDevice device; //creates the device object

	VkQueue graphicsQueue; //creates the graphics queue
	VkQueue presentQueue; //creates the presentation queue

	VkSwapchainKHR swapChain; //creates the swap chain object
	std::vector<VkImage> swapChainImages; //creates the vector of swap chain images
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews; //creates the vector of swap chain image views
	std::vector<VkFramebuffer> swapChainFramebuffers; //creates the vector of swap chain image buffers

	VkRenderPass renderPass; //creates the render pass
	VkDescriptorSetLayout descriptorSetLayout; //creates the object to contain the descriptor bindings
	VkPipelineLayout pipelineLayout; //creates the pipeline layout
	VkPipeline graphicsPipeline; //creates the graphics pipeline
	VkPipeline shellPipeline; //creates the shell pipeline
	VkPipeline finPipeline; //creates the fin pipeline
	VkPipeline shadowPipeline; //creates the shadow pipeline

	VkCommandPool commandPool; //creates the command pool

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	VkImage textureImage;
	VkImage textureImageFin;
	VkDeviceMemory textureImageMemory;
	VkDeviceMemory textureImageFinMemory;
	VkImageView textureImageView;
	VkImageView textureImageFinView;
	VkSampler textureSampler;

	std::vector<Vertex> vertices;
	std::vector<Vertex> quadVertices;
	std::vector<uint32_t> indices;
	std::vector<uint32_t> quadIndices;
	std::vector<VkBuffer> vertexBuffers;
	std::vector<VkBuffer> quadVertexBuffers;
	std::vector<VkDeviceMemory> vertexBuffersMemory;
	std::vector<VkDeviceMemory> quadVertexBuffersMemory;
	std::vector<VkBuffer> indexBuffers;
	std::vector<VkBuffer> quadIndexBuffers;
	std::vector<VkDeviceMemory> indexBuffersMemory;
	std::vector<VkDeviceMemory> quadIndexBuffersMemory;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	std::vector<VkBuffer> shadowUniformBuffers;
	std::vector<VkDeviceMemory> shadowUniformBuffersMemory;

	std::vector<VkBuffer> lightingBuffers;
	std::vector<VkDeviceMemory> lightingBuffersMemory;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	std::vector<VkCommandBuffer> commandBuffers; //creates the vector of command buffers

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	size_t currentFrame = 0; //counter for current frame

	bool framebufferResized = false;

	bool renderTexture = true;
	bool renderLighting = true;

	glm::mat4 modelMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(20.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	diredge::diredgeMesh mesh;

	void initWindow() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
		glfwSetKeyCallback(window, key_callback);
	}

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}

	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
		auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
		if (key == GLFW_KEY_T && action == GLFW_PRESS) {
			
			if (app->renderTexture) {
				app->renderTexture = false;
			}
			else {
				app->renderTexture = true;
			}
		}

		if (key == GLFW_KEY_L && action == GLFW_PRESS) {

			if (app->renderLighting) {
				app->renderLighting = false;
			}
			else {
				app->renderLighting = true;
			}
		}
	}

	void initVulkan() {
		createInstance(); //initialises vulkan library
		setupDebugMessenger(); //sets up the debug messenger
		createSurface(); //creates the surface
		pickPhysicalDevice(); //selects the physical device
		createLogicalDevice(); //creates the logical device
		createSwapChain(); //creates the swap chain
		createImageViews(); //creates the image views
		createRenderPass(); //creates the render pass
		createDescriptorSetLayout(); //creates the layout for the descriptor set
		createGraphicsPipeline(); //creates the graphics pipeline
		createShellPipeline(); //creates the shell pipeline
		createFinPipeline(); //creates the fin pipeline
		createShadowPipeline(); //creates the shadow pipeline
		createCommandPool(); //creates the command pool
		createDepthResources(); //creates the depth resources
		createFramebuffers(); //creates the frame buffers
		createTextureImage(); //creates the texture image
		createTextureImageView(); //creates the texture image view
		createTextureSampler(); //creates the texture samplers
		loadModel(); //loads the obj file
		createVertexBuffers(); //creates the vertex buffer
		createIndexBuffers(); //creates the index buffer
		createUniformBuffers(); //creates the uniform buffers
		createLightingBuffers(); //creates the lighting buffers
		createDescriptorPool(); //creates the descriptor pool
		createDescriptorSets(); //creates the descriptor sets
		createCommandBuffers(); //creates the command buffers
		createSyncObjects(); //creates the sync objects
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(window)) { //loops until window is closed by the user
			glfwPollEvents(); //checks for events
			drawFrame(); //calls the function to draw the frame
		}
		vkDeviceWaitIdle(device);
	}

	void cleanupSwapChain() {
		vkDestroyImageView(device, depthImageView, nullptr);
		vkDestroyImage(device, depthImage, nullptr);
		vkFreeMemory(device, depthImageMemory, nullptr);

		for (auto framebuffer : swapChainFramebuffers) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}

		vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipeline(device, shellPipeline, nullptr);
		vkDestroyPipeline(device, finPipeline, nullptr);
		vkDestroyPipeline(device, shadowPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);

		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(device, imageView, nullptr);
		}

		vkDestroySwapchainKHR(device, swapChain, nullptr);

		for (size_t i = 0; i < swapChainImages.size(); i++) {
			vkDestroyBuffer(device, uniformBuffers[i], nullptr);
			vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
			vkDestroyBuffer(device, shadowUniformBuffers[i], nullptr);
			vkFreeMemory(device, shadowUniformBuffersMemory[i], nullptr);
			vkDestroyBuffer(device, lightingBuffers[i], nullptr);
			vkFreeMemory(device, lightingBuffersMemory[i], nullptr);
		}

		vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	}

	void cleanup() {
		cleanupSwapChain();

		vkDestroySampler(device, textureSampler, nullptr);
		
		vkDestroyImageView(device, textureImageView, nullptr);
		vkDestroyImage(device, textureImage, nullptr);
		vkFreeMemory(device, textureImageMemory, nullptr);

		vkDestroyImageView(device, textureImageFinView, nullptr);
		vkDestroyImage(device, textureImageFin, nullptr);
		vkFreeMemory(device, textureImageFinMemory, nullptr);

		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		for (size_t i = 0; i < swapChainImages.size(); i++) {
			vkDestroyBuffer(device, indexBuffers[i], nullptr);
			vkFreeMemory(device, indexBuffersMemory[i], nullptr);

			vkDestroyBuffer(device, quadIndexBuffers[i], nullptr);
			vkFreeMemory(device, quadIndexBuffersMemory[i], nullptr);

			vkDestroyBuffer(device, vertexBuffers[i], nullptr);
			vkFreeMemory(device, vertexBuffersMemory[i], nullptr);

			vkDestroyBuffer(device, quadVertexBuffers[i], nullptr);
			vkFreeMemory(device, quadVertexBuffersMemory[i], nullptr);
		}
		

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

	void recreateSwapChain() {
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
		createShellPipeline();
		createFinPipeline();
		createShadowPipeline();
		createDepthResources();
		createFramebuffers();
		createUniformBuffers();
		createLightingBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createCommandBuffers();
	}

	void createInstance() {
		if (enableValidationLayers && !checkValidationLayerSupport()) { //if validation layers are enabled (i.e. in debug mode) and if any requested validation layers are not available
			throw std::runtime_error("validation layers requested, but not available!"); //throws runtime error
		}

		VkApplicationInfo appInfo = {}; //struct with information about application
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; //specifies structure type
		appInfo.pApplicationName = "Hello Triangle"; //sets the name of the application
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0); //specifies the version number of the application
		appInfo.pEngineName = "No Engine"; //specifies the name of the engine used (if any) in this case no engine is being used
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0); //specifies the version number of the engine used to create the application
		appInfo.apiVersion = VK_API_VERSION_1_0; //specifies the highest version of vulkan that the application is designed to use

		VkInstanceCreateInfo createInfo = {}; //struct that tells the vulkan driver which global extensions and validation layers we want to use
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; //specifies structure type
		createInfo.pApplicationInfo = &appInfo; //sets the application informaion for the instance

		auto extensions = getRequiredExtensions(); //gets the required extensions and stores them in a data structure
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size()); //sets the number of extensions
		createInfo.ppEnabledExtensionNames = extensions.data(); //sets the enabled extensions for the instance

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo; //creates the struct to be filled with information about the debug messenger
		if (enableValidationLayers) { //if validation layers are enabled
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size()); //sets number of enabled validation layers
			createInfo.ppEnabledLayerNames = validationLayers.data(); //includes the validation layer names

			populateDebugMessengerCreateInfo(debugCreateInfo); //calls the function to populatee the debug messenger
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo; //sets the pNext pointer to the extension specific structure
		}
		else { //if validation layers not enabled
			createInfo.enabledLayerCount = 0; //sets number of enabled layers to 0

			createInfo.pNext = nullptr; //sets pNext pointer to null
		}

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) { //attempts to create an instance however if function call returns negative VkResult then creation failed so error thrown 
			throw std::runtime_error("failed to create instance!"); //throws runtime error
		}
	}

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo = {}; //initialises struct with details about the messenger
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT; //specifies structure type
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT; //specifies types of severities callback should be called for
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT; //specifies types of messages callback is notified about
		createInfo.pfnUserCallback = debugCallback; //specifies pointer to the callback function
	}

	void setupDebugMessenger() {
		if (!enableValidationLayers) return; //if validation layers are not enabled then simply return without carrying out the rest of the function

		VkDebugUtilsMessengerCreateInfoEXT createInfo; //creates the struct to be filled with information about the messenger
		populateDebugMessengerCreateInfo(createInfo); //calls function to populate the debug messenger

		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) { //if debug messenger not successfully created
			throw std::runtime_error("failed to set up debug messenger!"); //throws runtime error
		}
	}

	void createSurface() {
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) { //if surface not successfully created
			throw std::runtime_error("failed to create window surface!"); //throws runtime error
		}
	}

	void pickPhysicalDevice() {
		uint32_t deviceCount = 0; //count for number of devices
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr); //counts the number of devices

		if (deviceCount == 0) { //if no devices found
			throw std::runtime_error("failed to find GPUs with Vulkan support!"); //throws runtime error
		}

		std::vector<VkPhysicalDevice> devices(deviceCount); //declares list for devices
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()); //adds devices to list

		for (const auto& device : devices) { //iterates through devices
			if (isDeviceSuitable(device)) { //calls function to check if device is suitable
				physicalDevice = device; //sets the suitable device as the physical device
				break; //ends current loop
			}
		}

		if (physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find a suitable GPU!"); //throws runtime error
		}
	}

	void createLogicalDevice() {
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice); //finds the queue families for the GPU

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos; //declares a vector of queue information structs
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		float queuePriority = 1.0f; //sets the priority of the queue
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo = {}; //struct containing information about the queue
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO; //specifies the struct type
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority; //sets the queue priority
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {}; //struct for device features
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo createInfo = {}; //struct for device information
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO; //specifies the struct type

		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &deviceFeatures; //sets the enabled features as the device features

		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()); //sets the extension count to the number of device extensions
		createInfo.ppEnabledExtensionNames = deviceExtensions.data(); //sets the names of the extensions

		if (enableValidationLayers) { //if validation layers are enabled
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size()); //sets the layer count to the number of validation layers
			createInfo.ppEnabledLayerNames = validationLayers.data(); //sets the names of the layers
		}
		else {
			createInfo.enabledLayerCount = 0; //sets the number of enabled validation layers to 0
		}

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) { //if device was not successfully created
			throw std::runtime_error("failed to create logical device!"); //throws runtime error
		}

		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue); //retrieves the queue handle for the graphics family 
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue); //retrieves the queue handle for the presentation family
	}

	void createSwapChain() {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice); //queries the swap chain details and stores them in a struct

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats); //calls the function to select the surface format from the available formats
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes); //calls the function to select the presentation mode from the available modes
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities); //calls the function to select the best swap extent

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1; //sets the image count to the minimum plus 1
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) { //if the count is greater than 0 and is less than the maximum image count
			imageCount = swapChainSupport.capabilities.maxImageCount; //sets the image count to the maximum image count
		}

		VkSwapchainCreateInfoKHR createInfo = {}; //struct for swap chain information
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR; //sets the type of the struct
		createInfo.surface = surface; //sets the surface information of the struct

		createInfo.minImageCount = imageCount; //sets the image count of the struct
		createInfo.imageFormat = surfaceFormat.format; //sets the surface format of the struct
		createInfo.imageColorSpace = surfaceFormat.colorSpace; //sets the color space of the struct
		createInfo.imageExtent = extent; //sets the extent of the struct
		createInfo.imageArrayLayers = 1; //sets the number of image array layers to 1
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; //specifies the image can be used to create a VkImageView suitable for use as a color or resolve attachment in a VkFrameBuffer

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice); //finds the queue families for the GPU
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() }; //creates an array storing both the graphics and presentation queue family indices

		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = VK_NULL_HANDLE; //sets old swap chain to null

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) { //if creation of swap chain unsuccessful
			throw std::runtime_error("failed to create swap chain!"); //throws runtime error
		}

		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	void createImageViews() {
		swapChainImageViews.resize(swapChainImages.size());

		for (size_t i = 0; i < swapChainImages.size(); i++) {
			swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		}
	}

	void createRenderPass() {
		VkAttachmentDescription colorAttachment = {}; //struct for color attachment information
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = findDepthFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRef = {}; //struct for color attachment reference information
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpasses[4] = {}; //struct for subpass information
		subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpasses[0].colorAttachmentCount = 1;
		subpasses[0].pColorAttachments = &colorAttachmentRef;
		subpasses[0].pDepthStencilAttachment = &depthAttachmentRef;

		subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpasses[1].colorAttachmentCount = 1;
		subpasses[1].pColorAttachments = &colorAttachmentRef;
		subpasses[1].pDepthStencilAttachment = &depthAttachmentRef;

		subpasses[2].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpasses[2].colorAttachmentCount = 1;
		subpasses[2].pColorAttachments = &colorAttachmentRef;
		subpasses[2].pDepthStencilAttachment = &depthAttachmentRef;

		subpasses[3].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpasses[3].colorAttachmentCount = 1;
		subpasses[3].pColorAttachments = &colorAttachmentRef;
		subpasses[3].pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency dependencies[4] = {}; //struct for dependency information
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = 0;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = 1;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].srcAccessMask = 0;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		dependencies[2].srcSubpass = 1;
		dependencies[2].dstSubpass = 2;
		dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[2].srcAccessMask = 0;
		dependencies[2].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[2].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		dependencies[3].srcSubpass = 2;
		dependencies[3].dstSubpass = 3;
		dependencies[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[3].srcAccessMask = 0;
		dependencies[3].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[3].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
		VkRenderPassCreateInfo renderPassInfo = {}; //struct for render pass information
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());;
		renderPassInfo.pAttachments = attachments.data();;
		renderPassInfo.subpassCount = 3;
		renderPassInfo.pSubpasses = subpasses;
		renderPassInfo.dependencyCount = 3;
		renderPassInfo.pDependencies = dependencies;

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!"); //throws runtime error
		}
	}

	void createDescriptorSetLayout() {
		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.pImmutableSamplers = nullptr;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutBinding lightingLayoutBinding = {};
		lightingLayoutBinding.binding = 1;
		lightingLayoutBinding.descriptorCount = 1;
		lightingLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		lightingLayoutBinding.pImmutableSamplers = nullptr;
		lightingLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
		samplerLayoutBinding.binding = 2;
		samplerLayoutBinding.descriptorCount = 2;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding shadowLayoutBinding = {};
		shadowLayoutBinding.binding = 3;
		shadowLayoutBinding.descriptorCount = 1;
		shadowLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		shadowLayoutBinding.pImmutableSamplers = nullptr;
		shadowLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		std::array<VkDescriptorSetLayoutBinding, 4> bindings = { uboLayoutBinding, lightingLayoutBinding, samplerLayoutBinding, shadowLayoutBinding };
		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	void createFinPipeline() {
		auto vertShaderCode = readFile("shaders/finvert.spv"); //stores the vertex shader path
		auto fragShaderCode = readFile("shaders/finfrag.spv"); //stores the fragment shader path

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode); //sets the vertex shader module by using the shader path
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode); //sets the fragment shader module using the shader path

		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {}; //struct for vertex shader stage information
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {}; //struct for fragment shader stage information
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo }; //array for the vertex/fragment shader stage information

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {}; //struct for vertex input information
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {}; //struct for input assembly information
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {}; //struct containing information about the viewport
		viewport.x = 0.0f; //sets the x position that the viewport starts from
		viewport.y = 0.0f; //sets the y position that the viewport starts from
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f; //sets the minimum depth of the viewport
		viewport.maxDepth = 1.0f; //sets the maximum depth of the viewport

		VkRect2D scissor = {}; //struct for scissor information
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState = {}; //struct for viewport state information
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer = {}; //struct for rasterizer information
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling = {}; //struct for multisampling information
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_FALSE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {}; //struct for color blend attachment information
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending = {}; //struct for color blending information
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {}; //struct for pipeline layout information
		VkPushConstantRange pushConstantInfo = { 0 };
		pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantInfo.offset = 0;
		pushConstantInfo.size = sizeof(float);

		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantInfo;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) { //if creation of pipeline layout unsuccessful
			throw std::runtime_error("failed to create pipeline layout!"); //throws runtime error
		}

		VkGraphicsPipelineCreateInfo pipelineInfo = {}; //struct for pipeline information
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 3;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &finPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!"); //throws runtime error
		}

		vkDestroyShaderModule(device, fragShaderModule, nullptr); //destroys the fragment shader module
		vkDestroyShaderModule(device, vertShaderModule, nullptr); //destroys the vertex shader module
	}

	void createShellPipeline() {
		auto vertShaderCode = readFile("shaders/shellvert.spv"); //stores the vertex shader path
		auto fragShaderCode = readFile("shaders/shellfrag.spv"); //stores the fragment shader path

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode); //sets the vertex shader module by using the shader path
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode); //sets the fragment shader module using the shader path

		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {}; //struct for vertex shader stage information
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {}; //struct for fragment shader stage information
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo }; //array for the vertex/fragment shader stage information

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {}; //struct for vertex input information
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {}; //struct for input assembly information
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {}; //struct containing information about the viewport
		viewport.x = 0.0f; //sets the x position that the viewport starts from
		viewport.y = 0.0f; //sets the y position that the viewport starts from
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f; //sets the minimum depth of the viewport
		viewport.maxDepth = 1.0f; //sets the maximum depth of the viewport

		VkRect2D scissor = {}; //struct for scissor information
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState = {}; //struct for viewport state information
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer = {}; //struct for rasterizer information
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling = {}; //struct for multisampling information
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {}; //struct for color blend attachment information
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending = {}; //struct for color blending information
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {}; //struct for pipeline layout information
		VkPushConstantRange pushConstantInfo = { 0 };
		pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantInfo.offset = 0;
		pushConstantInfo.size = sizeof(float);

		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantInfo;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) { //if creation of pipeline layout unsuccessful
			throw std::runtime_error("failed to create pipeline layout!"); //throws runtime error
		}

		VkGraphicsPipelineCreateInfo pipelineInfo = {}; //struct for pipeline information
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 2;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &shellPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!"); //throws runtime error
		}

		vkDestroyShaderModule(device, fragShaderModule, nullptr); //destroys the fragment shader module
		vkDestroyShaderModule(device, vertShaderModule, nullptr); //destroys the vertex shader module
	}

	void createGraphicsPipeline() {
		auto vertShaderCode = readFile("shaders/vert.spv"); //stores the vertex shader path
		auto fragShaderCode = readFile("shaders/frag.spv"); //stores the fragment shader path

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode); //sets the vertex shader module by using the shader path
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode); //sets the fragment shader module using the shader path

		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {}; //struct for vertex shader stage information
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {}; //struct for fragment shader stage information
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo }; //array for the vertex/fragment shader stage information

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {}; //struct for vertex input information
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {}; //struct for input assembly information
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {}; //struct containing information about the viewport
		viewport.x = 0.0f; //sets the x position that the viewport starts from
		viewport.y = 0.0f; //sets the y position that the viewport starts from
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f; //sets the minimum depth of the viewport
		viewport.maxDepth = 1.0f; //sets the maximum depth of the viewport

		VkRect2D scissor = {}; //struct for scissor information
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState = {}; //struct for viewport state information
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer = {}; //struct for rasterizer information
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling = {}; //struct for multisampling information
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {}; //struct for color blend attachment information
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending = {}; //struct for color blending information
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {}; //struct for pipeline layout information
		VkPushConstantRange pushConstantInfo = { 0 };
		pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantInfo.offset = 0;
		pushConstantInfo.size = sizeof(float);

		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantInfo;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) { //if creation of pipeline layout unsuccessful
			throw std::runtime_error("failed to create pipeline layout!"); //throws runtime error
		}

		VkGraphicsPipelineCreateInfo pipelineInfo = {}; //struct for pipeline information
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 1;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!"); //throws runtime error
		}

		vkDestroyShaderModule(device, fragShaderModule, nullptr); //destroys the fragment shader module
		vkDestroyShaderModule(device, vertShaderModule, nullptr); //destroys the vertex shader module
	}

	void createShadowPipeline() {
		auto vertShaderCode = readFile("shaders/shadowvert.spv"); //stores the vertex shader path
		auto fragShaderCode = readFile("shaders/shadowfrag.spv"); //stores the fragment shader path

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode); //sets the vertex shader module by using the shader path
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode); //sets the fragment shader module using the shader path

		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {}; //struct for vertex shader stage information
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {}; //struct for fragment shader stage information
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo }; //array for the vertex/fragment shader stage information

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {}; //struct for vertex input information
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {}; //struct for input assembly information
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {}; //struct containing information about the viewport
		viewport.x = 0.0f; //sets the x position that the viewport starts from
		viewport.y = 0.0f; //sets the y position that the viewport starts from
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f; //sets the minimum depth of the viewport
		viewport.maxDepth = 1.0f; //sets the maximum depth of the viewport

		VkRect2D scissor = {}; //struct for scissor information
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState = {}; //struct for viewport state information
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer = {}; //struct for rasterizer information
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling = {}; //struct for multisampling information
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {}; //struct for color blend attachment information
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending = {}; //struct for color blending information
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {}; //struct for pipeline layout information
		VkPushConstantRange pushConstantInfo = { 0 };
		pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantInfo.offset = 0;
		pushConstantInfo.size = sizeof(float);

		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantInfo;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) { //if creation of pipeline layout unsuccessful
			throw std::runtime_error("failed to create pipeline layout!"); //throws runtime error
		}

		VkGraphicsPipelineCreateInfo pipelineInfo = {}; //struct for pipeline information
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!"); //throws runtime error
		}

		vkDestroyShaderModule(device, fragShaderModule, nullptr); //destroys the fragment shader module
		vkDestroyShaderModule(device, vertShaderModule, nullptr); //destroys the vertex shader module
	}

	void createFramebuffers() {
		swapChainFramebuffers.resize(swapChainImageViews.size()); //gets number of image views and sets number of frame buffers

		for (size_t i = 0; i < swapChainImageViews.size(); i++) { //iterates through image views
			std::array<VkImageView, 2> attachments = {
				swapChainImageViews[i],
				depthImageView
			};

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!"); //throws runtime error
			}
		}
	}

	void createCommandPool() {
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

		VkCommandPoolCreateInfo poolInfo = {}; //struct for pool information
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
		//poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
	}

	void createDepthResources() {
		VkFormat depthFormat = findDepthFormat();

		createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
		depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	}

	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
		for (VkFormat format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}

		throw std::runtime_error("failed to find supported format!");
	}

	VkFormat findDepthFormat() {
		return findSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	bool hasStencilComponent(VkFormat format) {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	void createTextureImage() {
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(device, stagingBufferMemory);

		stbi_image_free(pixels);

		createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

		transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
		transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);

		pixels = stbi_load(FIN_TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		imageSize = texWidth * texHeight * 4;

		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}

		createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(device, stagingBufferMemory);

		stbi_image_free(pixels);

		createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImageFin, textureImageFinMemory);

		transitionImageLayout(textureImageFin, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyBufferToImage(stagingBuffer, textureImageFin, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
		transitionImageLayout(textureImageFin, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void createTextureImageView() {
		textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
		textureImageFinView = createImageView(textureImageFin, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	void createTextureSampler() {
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 16;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}
	}

	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}

		return imageView;
	}

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image!");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate image memory!");
		}

		vkBindImageMemory(device, image, imageMemory, 0);
	}

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else {
			throw std::invalid_argument("unsupported layout transition!");
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		endSingleTimeCommands(commandBuffer);
	}

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		endSingleTimeCommands(commandBuffer);
	}

	//checks which edges are silhouette edges and creates quads based on these vertices
	void createSilhouetteVertices() {
		std::unordered_map<Vertex, uint32_t> uniqueVertices = {};
		
		/*for (long i = 0; i < mesh.positions.size(); i++)
		{
			mesh.positions[i] = glm::vec3(glm::vec4(mesh.positions[i], 1.0) * modelMatrix);
			mesh.normals[i] = glm::vec3(glm::mat3(glm::transpose(glm::inverse(modelMatrix))) * mesh.normals[i]);
		}

		diredge::makeFaceNormals(mesh);*/

		quadVertices.clear();
		quadIndices.clear();

		for (long currentEdge = 0; currentEdge < (long)mesh.faceVertices.size(); currentEdge++) 
		{
			glm::vec3 vecA = { 30.0, 10.0, 30.0 };
			glm::vec3 vecB = mesh.positions[mesh.faceVertices[currentEdge]];
			glm::vec3 eyeVec = (vecA - vecB);
			eyeVec = glm::normalize(eyeVec);

			glm::vec3 faceNormA = mesh.faceNormals[currentEdge / 3];
			glm::vec3 faceNormB = mesh.faceNormals[mesh.otherHalf[currentEdge] / 3];

			float tempA = glm::dot(eyeVec, faceNormA);
			float tempB = glm::dot(eyeVec, faceNormB);

			bool test = (tempA * tempB) < 0;

			if (test)
			{
				Vertex vertexA = {};
				Vertex vertexB = {};
				Vertex vertexC = {};
				Vertex vertexD = {};

				vertexA.pos = mesh.positions[mesh.faceVertices[currentEdge]];
				vertexA.color = { 1.0f, 1.0f, 1.0f };
				vertexA.texCoord = { 0.0 , 1.0 };
				vertexA.normal = eyeVec;

				vertexB.pos = mesh.positions[mesh.faceVertices[NEXT_EDGE(currentEdge)]];
				vertexB.color = { 1.0f, 1.0f, 1.0f };
				vertexB.texCoord = { 1.0 , 1.0 };
				vertexB.normal = eyeVec;

				vertexC.pos = (mesh.positions[mesh.faceVertices[currentEdge]] + (mesh.normals[mesh.faceVertices[currentEdge]]));
				vertexC.color = { 1.0f, 1.0f, 1.0f };
				vertexC.texCoord = { 0.0 , 0.0 };
				vertexC.normal = eyeVec;

				vertexD.pos = (mesh.positions[mesh.faceVertices[NEXT_EDGE(currentEdge)]] + (mesh.normals[mesh.faceVertices[NEXT_EDGE(currentEdge)]]));
				vertexD.color = { 1.0f, 1.0f, 1.0f };
				vertexD.texCoord = { 1.0 , 0.0 };
				vertexD.normal = eyeVec;

				if (uniqueVertices.count(vertexA) == 0) {
					uniqueVertices[vertexA] = static_cast<uint32_t>(quadVertices.size());
					quadVertices.push_back(vertexA);
				}

				quadIndices.push_back(uniqueVertices[vertexA]);

				if (uniqueVertices.count(vertexB) == 0) {
					uniqueVertices[vertexB] = static_cast<uint32_t>(quadVertices.size());
					quadVertices.push_back(vertexB);
				}

				quadIndices.push_back(uniqueVertices[vertexB]);

				if (uniqueVertices.count(vertexC) == 0) {
					uniqueVertices[vertexC] = static_cast<uint32_t>(quadVertices.size());
					quadVertices.push_back(vertexC);
				}

				quadIndices.push_back(uniqueVertices[vertexC]);

				quadIndices.push_back(uniqueVertices[vertexB]);

				if (uniqueVertices.count(vertexD) == 0) {
					uniqueVertices[vertexD] = static_cast<uint32_t>(quadVertices.size());
					quadVertices.push_back(vertexD);
				}

				quadIndices.push_back(uniqueVertices[vertexD]);

				quadIndices.push_back(uniqueVertices[vertexC]);
			}
		}
		diredge::restoreDefaults(mesh);
	}

	void loadModel() {
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
			throw std::runtime_error(warn + err);
		}

		std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex = {};

				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				if (MODEL_PATH == "models/bunny.obj")
				{
					vertex.pos *= 200.0f;
				}
				
				vertex.color = { 1.0f, 1.0f, 1.0f };

				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

				vertex.normal = { 
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[vertex]);
			}
		}
		std::vector<glm::vec3> positions;
		for (long i = 0; i < vertices.size(); i++)
		{
			positions.push_back(vertices[i].pos);
		}

		std::vector<glm::vec3> normals;
		for (long i = 0; i < vertices.size(); i++)
		{
			normals.push_back(vertices[i].normal);
		}

		mesh = diredge::createMesh(positions, normals, indices);

		createSilhouetteVertices();
	}

	void createVertexBuffers() {
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		vertexBuffers.resize(swapChainImages.size());
		vertexBuffersMemory.resize(swapChainImages.size());

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		for (size_t i = 0; i < swapChainImages.size(); i++) {
			createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffers[i], vertexBuffersMemory[i]);

			copyBuffer(stagingBuffer, vertexBuffers[i], bufferSize);
		}

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);

		bufferSize = sizeof(quadVertices[0]) * quadVertices.size();

		quadVertexBuffers.resize(swapChainImages.size());
		quadVertexBuffersMemory.resize(swapChainImages.size());

		VkBuffer stagingBufferQuads;
		VkDeviceMemory stagingBufferMemoryQuads;

		bufferSize = sizeof(quadVertices[0]) * quadVertices.size();

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBufferQuads, stagingBufferMemoryQuads);

		vkMapMemory(device, stagingBufferMemoryQuads, 0, bufferSize, 0, &data);
		memcpy(data, quadVertices.data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemoryQuads);

		for (size_t i = 0; i < swapChainImages.size(); i++) {
			createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, quadVertexBuffers[i], quadVertexBuffersMemory[i]);

			copyBuffer(stagingBufferQuads, quadVertexBuffers[i], bufferSize);
		}

		vkDestroyBuffer(device, stagingBufferQuads, nullptr);
		vkFreeMemory(device, stagingBufferMemoryQuads, nullptr);
	}

	void updateSilhouetteVertexBuffers(uint32_t imageIndex) {
		VkDeviceSize bufferSize = sizeof(quadVertices[0]) * quadVertices.size();

		VkBuffer stagingBufferQuads;
		VkDeviceMemory stagingBufferMemoryQuads;

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBufferQuads, stagingBufferMemoryQuads);

		void* data;
		vkMapMemory(device, stagingBufferMemoryQuads, 0, bufferSize, 0, &data);
		memcpy(data, quadVertices.data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemoryQuads);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, quadVertexBuffers[imageIndex], quadVertexBuffersMemory[imageIndex]);

		copyBuffer(stagingBufferQuads, quadVertexBuffers[imageIndex], bufferSize);

		vkDestroyBuffer(device, stagingBufferQuads, nullptr);
		vkFreeMemory(device, stagingBufferMemoryQuads, nullptr);
	}

	void createIndexBuffers() {
		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		indexBuffers.resize(swapChainImages.size());
		indexBuffersMemory.resize(swapChainImages.size());

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		for (size_t i = 0; i < swapChainImages.size(); i++) {
			createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffers[i], indexBuffersMemory[i]);

			copyBuffer(stagingBuffer, indexBuffers[i], bufferSize);
		}

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);

		bufferSize = sizeof(quadIndices[0]) * quadIndices.size();

		quadIndexBuffers.resize(swapChainImages.size());
		quadIndexBuffersMemory.resize(swapChainImages.size());

		VkBuffer stagingBufferQuads;
		VkDeviceMemory stagingBufferMemoryQuads;

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBufferQuads, stagingBufferMemoryQuads);

		vkMapMemory(device, stagingBufferMemoryQuads, 0, bufferSize, 0, &data);
		memcpy(data, quadIndices.data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemoryQuads);

		for (size_t i = 0; i < swapChainImages.size(); i++) {
			createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, quadIndexBuffers[i], quadIndexBuffersMemory[i]);

			copyBuffer(stagingBufferQuads, quadIndexBuffers[i], bufferSize);
		}

		vkDestroyBuffer(device, stagingBufferQuads, nullptr);
		vkFreeMemory(device, stagingBufferMemoryQuads, nullptr);
	}

	void updateSilhouetteIndexBuffers(uint32_t imageIndex) {
		VkDeviceSize bufferSize = sizeof(quadIndices[0]) * quadIndices.size();

		VkBuffer stagingBufferQuads;
		VkDeviceMemory stagingBufferMemoryQuads;

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBufferQuads, stagingBufferMemoryQuads);

		void* data;
		vkMapMemory(device, stagingBufferMemoryQuads, 0, bufferSize, 0, &data);
		memcpy(data, quadIndices.data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemoryQuads);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, quadIndexBuffers[imageIndex], quadIndexBuffersMemory[imageIndex]);

		copyBuffer(stagingBufferQuads, quadIndexBuffers[imageIndex], bufferSize);

		vkDestroyBuffer(device, stagingBufferQuads, nullptr);
		vkFreeMemory(device, stagingBufferMemoryQuads, nullptr);
	}

	void createUniformBuffers() {
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		uniformBuffers.resize(swapChainImages.size());
		uniformBuffersMemory.resize(swapChainImages.size());

		for (size_t i = 0; i < swapChainImages.size(); i++) {
			createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
		}

		bufferSize = sizeof(ShadowBufferObject);

		shadowUniformBuffers.resize(swapChainImages.size());
		shadowUniformBuffersMemory.resize(swapChainImages.size());

		for (size_t i = 0; i < swapChainImages.size(); i++) {
			createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, shadowUniformBuffers[i], shadowUniformBuffersMemory[i]);
		}
	}

	void createLightingBuffers() {
		VkDeviceSize bufferSize = sizeof(LightingConstants);

		lightingBuffers.resize(swapChainImages.size());
		lightingBuffersMemory.resize(swapChainImages.size());

		for (size_t i = 0; i < swapChainImages.size(); i++) {
			createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, lightingBuffers[i], lightingBuffersMemory[i]);
		}
	}

	void createDescriptorPool() {
		std::array<VkDescriptorPoolSize, 4> poolSizes = {};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[2].descriptorCount = static_cast<uint32_t>(swapChainImages.size()) * 2;
		poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[3].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());

		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	void createDescriptorSets() {
		std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
		allocInfo.pSetLayouts = layouts.data();

		descriptorSets.resize(swapChainImages.size());
		if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < swapChainImages.size(); i++) {

			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorBufferInfo shadowBufferInfo = {};
			shadowBufferInfo.buffer = shadowUniformBuffers[i];
			shadowBufferInfo.offset = 0;
			shadowBufferInfo.range = sizeof(ShadowBufferObject);

			VkDescriptorBufferInfo lightingBufferInfo = {};
			lightingBufferInfo.buffer = lightingBuffers[i];
			lightingBufferInfo.offset = 0;
			lightingBufferInfo.range = sizeof(LightingConstants);

			VkDescriptorImageInfo imageInfo[2];
			imageInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo[0].imageView = textureImageView;
			imageInfo[0].sampler = textureSampler;

			imageInfo[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo[1].imageView = textureImageFinView;
			imageInfo[1].sampler = textureSampler;

			std::array<VkWriteDescriptorSet, 4> descriptorWrites = {};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pBufferInfo = &lightingBufferInfo;

			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = descriptorSets[i];
			descriptorWrites[2].dstBinding = 2;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[2].descriptorCount = 2;
			descriptorWrites[2].pImageInfo = imageInfo;

			descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[3].dstSet = descriptorSets[i];
			descriptorWrites[3].dstBinding = 3;
			descriptorWrites[3].dstArrayElement = 0;
			descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[3].descriptorCount = 1;
			descriptorWrites[3].pBufferInfo = &shadowBufferInfo;

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

	VkCommandBuffer beginSingleTimeCommands() {
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkBufferCopy copyRegion = {};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	void createCommandBuffers() {
		commandBuffers.resize(swapChainFramebuffers.size()); //gets number of frame buffers

		VkCommandBufferAllocateInfo allocInfo = {}; //struct for command buffer allocation information
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

		if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) { //if command buffers were not successfully allocated
			throw std::runtime_error("failed to allocate command buffers!"); //throws runtime error
		}

		for (size_t i = 0; i < commandBuffers.size(); i++) { //iterates through command buffers
			VkCommandBufferBeginInfo beginInfo = {}; //struct for command buffer beginning information
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
				throw std::runtime_error("failed to begin recording command buffer!"); //throws runtime error
			}

			VkRenderPassBeginInfo renderPassInfo = {}; //struct for render pass information
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = swapChainFramebuffers[i];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = swapChainExtent;

			std::array<VkClearValue, 2> clearValues = {};
			clearValues[0].color = { 0.16f, 0.56f, 0.81f, 1.0f };
			clearValues[1].depthStencil = { 1.0f, 0 };

			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();

			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);

			VkBuffer vertexBuff[] = { vertexBuffers[i] };
			VkDeviceSize offsets[] = { 0 };

			//Shadow pass
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuff, offsets);

			vkCmdBindIndexBuffer(commandBuffers[i], indexBuffers[i], 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

			vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

			vkCmdNextSubpass(commandBuffers[i], VK_SUBPASS_CONTENTS_INLINE);

			//Base pass
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuff, offsets);

			vkCmdBindIndexBuffer(commandBuffers[i], indexBuffers[i], 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

			vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

			vkCmdNextSubpass(commandBuffers[i], VK_SUBPASS_CONTENTS_INLINE);

			//Fin pass
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, finPipeline);

			VkBuffer quadVertexBuff[] = { quadVertexBuffers[i] };

			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, quadVertexBuff, offsets);

			vkCmdBindIndexBuffer(commandBuffers[i], quadIndexBuffers[i], 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

			//vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(quadIndices.size()), 1, 0, 0, 0);

			vkCmdNextSubpass(commandBuffers[i], VK_SUBPASS_CONTENTS_INLINE);
			
			//Shell pass
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, shellPipeline);

			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuff, offsets);

			vkCmdBindIndexBuffer(commandBuffers[i], indexBuffers[i], 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

			float currentLayer = 0.0f;
			float maxLayer = 1.0f;
			float noOfLayers = 40.0f;

			vkCmdPushConstants(commandBuffers[i], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(currentLayer), &currentLayer);

			while (currentLayer <= maxLayer)
			{
				//vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
				currentLayer += (maxLayer / noOfLayers);
				vkCmdPushConstants(commandBuffers[i], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(currentLayer), &currentLayer);
			}
			
			vkCmdEndRenderPass(commandBuffers[i]);

			if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}
		}
	}

	void createSyncObjects() {
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo = {}; //struct for semaphore information
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {}; //struct for fence information
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create synchronization objects for a frame!"); //throws runtime error
			}
		}
	}

	void updateUniformBuffer(uint32_t currentImage) {
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo = {};
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix = glm::rotate(glm::mat4(1.0f), time * glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		ubo.view = glm::lookAt(glm::vec3(30.0f, 10.0f, 30.0f), glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		ubo.proj = glm::perspective(glm::radians(90.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 300.0f);
		ubo.proj[1][1] *= -1;
		ubo.renderTex = 1.0f;
		if (!renderTexture) {
			ubo.renderTex = 0.0f;
		}

		void* data;
		vkMapMemory(device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, uniformBuffersMemory[currentImage]);

		ShadowBufferObject shadow = {};
		ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		ubo.view = glm::lookAt(glm::vec3(20.0f, 40.0f, 50.0f), glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		ubo.proj = glm::perspective(glm::radians(90.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 300.0f);
		ubo.proj[1][1] *= -1;

		vkMapMemory(device, shadowUniformBuffersMemory[currentImage], 0, sizeof(shadow), 0, &data);
		memcpy(data, &shadow, sizeof(shadow));
		vkUnmapMemory(device, shadowUniformBuffersMemory[currentImage]);

		LightingConstants lighting = {};
		if (renderLighting) {
			lighting.lightPosition = glm::vec3(20.0f, 40.0f, 50.0f);
			lighting.lightAmbient = glm::vec3(0.8f, 0.8f, 0.8f);
			lighting.lightDiffuse = glm::vec3(1.0f, 1.0f, 1.0f);
			lighting.lightSpecular = glm::vec3(0.288f, 0.288f, 0.288f);
			lighting.lightSpecularExponent = 28.0f;
		}
		else {
			lighting.lightPosition = glm::vec3(20.0f, 40.0f, 70.0f);
			lighting.lightAmbient = glm::vec3(0.0f, 0.0f, 0.0f);
			lighting.lightDiffuse = glm::vec3(0.0f, 0.0f, 0.0f);
			lighting.lightSpecular = glm::vec3(0.0f, 0.0f, 0.0f);
			lighting.lightSpecularExponent = 0.0f;
		}
		
		vkMapMemory(device, lightingBuffersMemory[currentImage], 0, sizeof(lighting), 0, &data);
		memcpy(data, &lighting, sizeof(lighting));
		vkUnmapMemory(device, lightingBuffersMemory[currentImage]);
	}

	void drawFrame() {
		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		updateUniformBuffer(imageIndex);
		createSilhouetteVertices();
		updateSilhouetteVertexBuffers(imageIndex);
		updateSilhouetteIndexBuffers(imageIndex);
		vkResetCommandPool(device, commandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
		createCommandBuffers();

		if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
			vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
		}
		imagesInFlight[imageIndex] = inFlightFences[currentFrame];

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkResetFences(device, 1, &inFlightFences[currentFrame]);

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

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

	VkShaderModule createShaderModule(const std::vector<char>& code) {
		VkShaderModuleCreateInfo createInfo = {}; //struct containing information about the shader module
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO; //sets struct type
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule; //declares a shader module object
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) { //if shader module not created successfully
			throw std::runtime_error("failed to create shader module!"); //throws runtime error
		}

		return shaderModule; //returns the shader module
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		for (const auto& availablePresentMode : availablePresentModes) { //iterates through the available presentation modes
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

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

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
		SwapChainSupportDetails details; //declares struct for swap chain support details

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities); //gets the surface capabilites of the device

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0) { //if there are any available presentation modes
			details.presentModes.resize(presentModeCount); //sets the number of presenation modes
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data()); //gets the surface presentation modes of the device
		}

		return details; //returns the details of the swap chain support
	}

	bool isDeviceSuitable(VkPhysicalDevice device) {
		QueueFamilyIndices indices = findQueueFamilies(device);  //gets the queue families for the device

		bool extensionsSupported = checkDeviceExtensionSupport(device); //calls function to check the extension support for the device

		bool swapChainAdequate = false; //sets boolean to false
		if (extensionsSupported) { //if the extensions are supported
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device); //checks swap chain support for the device
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty(); //checks if the swap chain is adequate
		}

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

		return indices.isComplete() && extensionsSupported && swapChainAdequate; //returns whether the device is suitable or not
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
		uint32_t extensionCount; //declares count for number of extensions
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr); //retrieves a list of supported extensions

		std::vector<VkExtensionProperties> availableExtensions(extensionCount); //allocates an array to hold extension details
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data()); //queries the extension details

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end()); //set containing the required extensions

		for (const auto& extension : availableExtensions) { //iterates through array of available extensions
			requiredExtensions.erase(extension.extensionName); //removes the extension from the required extensions
		}

		return requiredExtensions.empty(); //returns true if list of required extensions is empty and false otherwise
	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices; //declares indices for queue family

		uint32_t queueFamilyCount = 0; //sets the count for the number of queue families to 0
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr); //gets the number of queue families for the device

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount); //vector of queue family properties
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data()); //gets the queue families for the device

		int i = 0; //sets i to 0
		for (const auto& queueFamily : queueFamilies) { //iterates through the queue families
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) { //if the queues in the family support graphics operations
				indices.graphicsFamily = i; //sets the graphics family of the indices to i
			}

			VkBool32 presentSupport = false; //sets presentation support to false
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport); //checks if there is presentation support for the device

			if (presentSupport) { //if there is presentation support
				indices.presentFamily = i; //sets the presentation family of the indices to i
			}

			if (indices.isComplete()) { //if the indices is complete
				break; //ends the current loop
			}

			i++; //updates i value
		}

		return indices; //returns the indices
	}

	std::vector<const char*> getRequiredExtensions() {
		uint32_t glfwExtensionCount = 0; //tracks number of extensions
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount); //returns extensions needed to interface with the window system and updates number of extensions

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount); //declares a list for storing the extensions

		if (enableValidationLayers) { //if validation layers enabled
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); //adds the extension to the list
		}

		return extensions; //returns the list of extensions
	}

	bool checkValidationLayerSupport() {
		uint32_t layerCount; //variable to hold number of available layers
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr); //gets number of available layers

		std::vector<VkLayerProperties> availableLayers(layerCount); //creates a list to store the available layers
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()); //stores the available layers in the list

		for (const char* layerName : validationLayers) { //iterates through validation layers
			bool layerFound = false; //boolean storing whether the current validation layer has been found in the available layers

			for (const auto& layerProperties : availableLayers) { //iterates through available layers
				if (strcmp(layerName, layerProperties.layerName) == 0) { //checks if both strings are equal
					layerFound = true; //sets boolean to true indicating that the layer is available
					break; //ends current loop
				}
			}

			if (!layerFound) { //if the current layer is not available
				return false; //returns false boolean indicating insufficient validation layer support
			}
		}

		return true; //returns true indicating that all required validation layers are available
	}

	static std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) { //if the file fails to open
			throw std::runtime_error("failed to open file!"); //throws runtime error
		}

		size_t fileSize = (size_t)file.tellg(); //gets the size of the file
		std::vector<char> buffer(fileSize); //creates a vector of chars based on the size of the file

		file.seekg(0); //goes to the start of the information
		file.read(buffer.data(), fileSize); //read the file into the buffer

		file.close(); //closes the file stream

		return buffer; //returns the char buffer
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}
};

int main() {
	HelloTriangleApplication app; //creates the application

	try {
		app.run(); //runs the application
	}
	catch (const std::exception& e) { //catches any exception
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}