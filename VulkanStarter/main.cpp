#define GLFW_INCLUDE_VULKAN //automatically loads vulkan header with GLFW
#include <GLFW/glfw3.h> //includes GLFW definitions

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

const int WIDTH = 800; //constant value for width of window
const int HEIGHT = 600; //constant value for height of window

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
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
	VkPipelineLayout pipelineLayout; //creates the pipeline layout
	VkPipeline graphicsPipeline; //creates the graphics pipeline

	VkCommandPool commandPool; //creates the command pool
	std::vector<VkCommandBuffer> commandBuffers; //creates the vector of command buffers

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	size_t currentFrame = 0; //counter for current frame

	void initWindow() {
		glfwInit(); //initialises the GLFW library

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //tells GLFW to not create an OpenGL context
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); //disables resizing of window as this requires special handling

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr); //initalises the window (width, height, title, specify which monitor, only relevant to OpenGL)
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
		createGraphicsPipeline(); //creates the graphics pipeline
		createFramebuffers(); //creates the frame buffers
		createCommandPool(); //creates the command pool
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

	void cleanup() {
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}

		vkDestroyCommandPool(device, commandPool, nullptr); //destroys the command pool

		for (auto framebuffer : swapChainFramebuffers) { //iterates through the frame buffers
			vkDestroyFramebuffer(device, framebuffer, nullptr); //destroys each frame buffer
		}

		vkDestroyPipeline(device, graphicsPipeline, nullptr); //destroys the graphics pipeline
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr); //destroys the pipeline layout
		vkDestroyRenderPass(device, renderPass, nullptr); //destroys the render pass

		for (auto imageView : swapChainImageViews) { //iterates through the image views
			vkDestroyImageView(device, imageView, nullptr); //destroys each image view
		}

		vkDestroySwapchainKHR(device, swapChain, nullptr); //destroys the swap chain
		vkDestroyDevice(device, nullptr); //destroys the device

		if (enableValidationLayers) { //if validation layers enabled
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr); //destroys debug messenger
		}

		vkDestroySurfaceKHR(instance, surface, nullptr); //destroys the surface
		vkDestroyInstance(instance, nullptr); //destroys the instance

		glfwDestroyWindow(window); //destroys the window object

		glfwTerminate(); //terminates GLFW
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

		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!"); //throws runtime error
		}

		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	void createImageViews() {
		swapChainImageViews.resize(swapChainImages.size()); //sets number of swap chain images

		for (size_t i = 0; i < swapChainImages.size(); i++) { //iterates through swap chain images
			VkImageViewCreateInfo createInfo = {}; //struct for image information
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
				throw std::runtime_error("failed to create image views!"); //throws runtime error
			}
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

		VkAttachmentReference colorAttachmentRef = {}; //struct for color attachment reference information
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {}; //struct for subpass information
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency = {}; //struct for dependency information
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo = {}; //struct for render pass information
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!"); //throws runtime error
		}
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
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;

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
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling = {}; //struct for multisampling information
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {}; //struct for color blend attachment information
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

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
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pushConstantRangeCount = 0;

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
			VkImageView attachments[] = {
				swapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo = {}; //struct for frame buffer information
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
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

		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
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

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = swapChainFramebuffers[i];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = swapChainExtent;

			VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

			vkCmdEndRenderPass(commandBuffers[i]);

			if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!"); //throws runtime error
			}
		}
	}

	void createSyncObjects() {
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
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

	void drawFrame() {
		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

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
			throw std::runtime_error("failed to submit draw command buffer!"); //throws runtime error
		}

		VkPresentInfoKHR presentInfo = {}; //struct containing information about the presentation
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR; //specifies struct type

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1; //sets count for number of swap chains to 1
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = &imageIndex;

		vkQueuePresentKHR(presentQueue, &presentInfo);

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
			VkExtent2D actualExtent = { WIDTH, HEIGHT };

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