#include "core/device.hpp"

#include "platform/vulkan/device_vk.hpp"

#include "VkBootstrap.h"
#define VOLK_IMPLEMENTATION
#include "engine.hpp"
#include "volk.h"

#include "platform/vulkan/images_vk.hpp"

#include <chrono>

#include <SDL3/SDL.h>

#include <SDL3/SDL_vulkan.h>

#include <platform/vulkan/initializers_vk.hpp>

#include <platform/vulkan/types_vk.hpp>

#include "core/fileio.hpp"
#include "external/imgui_impl.hpp"
#include "platform/vulkan/imgui_impl_vk.hpp"

#include "platform/vulkan/loader_vk.hpp"

#include "platform/vulkan/pipelines_vk.hpp"

#include "utility/logger.hpp"

#include <glm/glm.hpp>

#include <algorithm>

struct ImGui_ImplVulkan_InitInfo;
namespace hm::internal
{

bool stop_rendering {false};

// immediate submit structures
VkFence _immFence;
VkCommandBuffer _immCommandBuffer;
VkCommandPool _immCommandPool;

void init_swapchain(glm::uvec2 windowSize);
void init_commands();
void init_sync_structures();

void create_swapchain(uint32_t width, uint32_t height);

void destroy_swapchain();

} // namespace hm::internal
using namespace hm;
using namespace hm::internal;
// Default debug messenger
// Feel free to copy-paste it into your own code, change it as needed, then call
// `set_debug_callback()` to use that instead
VkBool32 VulkanCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
{
  const auto* const mt = vkb::to_string_message_type(messageType);

  switch (messageSeverity)
  {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      log::Error("{} {}", mt, pCallbackData->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      log::Warning("{}  {}", mt, pCallbackData->pMessage);
      break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      log::Info("{}  {}", mt, pCallbackData->pMessage);
      break;

    default:
      log::Info("Unknown Severity");

      log::Info("{}  {}", mt, pCallbackData->pMessage);
      break;
  }

  return VK_FALSE; // Applications must return false here
}

void Device::Initialize()
{
  constexpr SDL_WindowFlags windowFlags =
      SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;

  m_windowSize = {m_windowSize.x, m_windowSize.y};
  m_pWindow = SDL_CreateWindow(WindowTitle, static_cast<i32>(m_windowSize.x),
                               static_cast<i32>(m_windowSize.y), windowFlags);

  InitVulkan(m_pWindow, m_bValidationLayer);
  init_swapchain(m_windowSize);
  init_commands();
  init_sync_structures();

  external::ImGuiInitialize();
  external::ImGuiInitializeVulkan(m_pWindow);
}

void Device::DestroyBackend()
{
  external::ImGuiCleanUp();

  // make sure the gpu has stopped doing its things
  vkDeviceWaitIdle(_device);

  // free per-frame structures and deletion queue
  for (int i = 0; i < FRAME_OVERLAP; i++)
  {
    vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);

    // destroy sync objects
    vkDestroyFence(_device, _frames[i]._renderFence, nullptr);
    vkDestroySemaphore(_device, _frames[i]._renderSemaphore, nullptr);
    vkDestroySemaphore(_device, _frames[i]._swapchainSemaphore, nullptr);

    _frames[i]._deletionQueue.flush();
  }

  // flush the global deletion queue
  _mainDeletionQueue.flush();

  SDL_DestroyWindow(m_pWindow);

  destroy_swapchain();

  vkDestroySurfaceKHR(_instance, _surface, nullptr);
  vkDestroyDevice(_device, nullptr);

  vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
  vkDestroyInstance(_instance, nullptr);
}

// void Device::PreRender()
//{
//   external::ImGuiStartFrame();
//   ChangeGraphicsBackend();
// }

Device::~Device()
{
  DestroyBackend();
}
void Device::EndFrame() {}

AllocatedImage hm::create_image(VkExtent3D size, VkFormat format,
                                VkImageUsageFlags usage, bool mipmapped)
{
  AllocatedImage newImage;
  newImage.imageFormat = format;
  newImage.imageExtent = size;

  VkImageCreateInfo img_info = vkinit::image_create_info(format, usage, size);
  if (mipmapped)
  {
    img_info.mipLevels = static_cast<uint32_t>(std::floor(
                             std::log2(std::max(size.width, size.height)))) +
                         1;
  }

  // always allocate images on dedicated GPU memory
  VmaAllocationCreateInfo allocinfo = {};
  allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  allocinfo.requiredFlags =
      VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  // allocate and create the image
  VK_CHECK(vmaCreateImage(_allocator, &img_info, &allocinfo, &newImage.image,
                          &newImage.allocation, nullptr));

  // if the format is a depth format, we will need to have it use the correct
  // aspect flag
  VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
  if (format == VK_FORMAT_D32_SFLOAT)
  {
    aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
  }

  // build a image-view for the image
  VkImageViewCreateInfo view_info =
      vkinit::imageview_create_info(format, newImage.image, aspectFlag);
  view_info.subresourceRange.levelCount = img_info.mipLevels;

  VK_CHECK(
      vkCreateImageView(_device, &view_info, nullptr, &newImage.imageView));

  return newImage;
}
AllocatedImage hm::create_image(void* data, VkExtent3D size, VkFormat format,
                                VkImageUsageFlags usage, bool mipmapped)
{
  size_t data_size = size.depth * size.width * size.height * 4;
  AllocatedBuffer uploadbuffer = create_buffer(
      data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  memcpy(uploadbuffer.info.pMappedData, data, data_size);

  AllocatedImage new_image = create_image(
      size, format,
      usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      mipmapped);

  immediate_submit(
      [&](VkCommandBuffer cmd)
      {
        vkutil::transition_image(cmd, new_image.image,
                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = size;

        // copy the buffer into the image
        vkCmdCopyBufferToImage(cmd, uploadbuffer.buffer, new_image.image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                               &copyRegion);

        if (mipmapped)
        {
          vkutil::generate_mipmaps(cmd, new_image.image,
                                   VkExtent2D {new_image.imageExtent.width,
                                               new_image.imageExtent.height});
        }
        else
        {
          vkutil::transition_image(cmd, new_image.image,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
      });

  destroy_buffer(uploadbuffer);

  return new_image;
}
void hm::destroy_image(const AllocatedImage& img)
{
  vkDestroyImageView(_device, img.imageView, nullptr);
  vmaDestroyImage(_allocator, img.image, img.allocation);
}
FrameData& internal::get_current_frame()
{
  return _frames[_frameNumber % FRAME_OVERLAP];
}

MaterialInstance GLTFMetallic_Roughness::write_material(
    VkDevice device, MaterialPass pass, const MaterialResources& resources,
    DescriptorAllocatorGrowable& descriptorAllocator)
{
  MaterialInstance matData;
  matData.passType = pass;
  if (pass == MaterialPass::Transparent)
  {
    matData.pipeline = &transparentPipeline;
  }
  else
  {
    matData.pipeline = &opaquePipeline;
  }

  matData.materialSet = descriptorAllocator.allocate(device, materialLayout);

  writer.clear();
  writer.write_buffer(0, resources.dataBuffer, sizeof(MaterialConstants),
                      resources.dataBufferOffset,
                      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
  writer.write_image(1, resources.colorImage.imageView, resources.colorSampler,
                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
  writer.write_image(2, resources.metalRoughImage.imageView,
                     resources.metalRoughSampler,
                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

  writer.update_set(device, matData.materialSet);

  return matData;
}

void internal::InitVulkan(SDL_Window* window, bool debug)
{
  vkb::InstanceBuilder builder;

  // make the vulkan instance, with basic debug features
  auto inst_ret = builder.set_app_name("Example Vulkan Application")
                      .request_validation_layers(debug)
                      .set_debug_callback(VulkanCallback)
                      .require_api_version(1, 3, 0)
                      .build();

  vkb::Instance vkb_inst = inst_ret.value();

  volkInitialize();
  volkLoadInstance(vkb_inst);
  // grab the instance
  _instance = vkb_inst.instance;
  _debug_messenger = vkb_inst.debug_messenger;

  // device
  SDL_Vulkan_CreateSurface(window, _instance, nullptr, &_surface);

  // vulkan 1.3 features
  VkPhysicalDeviceVulkan13Features features {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
  features.dynamicRendering = true;
  features.synchronization2 = true;

  // vulkan 1.2 features
  VkPhysicalDeviceVulkan12Features features12 {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
  features12.bufferDeviceAddress = true;
  features12.descriptorIndexing = true;

  // use vkbootstrap to select a gpu.
  // We want a gpu that can write to the SDL surface and supports vulkan 1.3
  // with the correct features
  vkb::PhysicalDeviceSelector selector {vkb_inst};
  vkb::PhysicalDevice physicalDevice = selector.set_minimum_version(1, 3)
                                           .set_required_features_13(features)
                                           .set_required_features_12(features12)
                                           .set_surface(_surface)
                                           .select()
                                           .value();

  // create the final vulkan device
  vkb::DeviceBuilder deviceBuilder {physicalDevice};

  vkb::Device vkbDevice = deviceBuilder.build().value();

  if (vkbDevice && vkbDevice.physical_device)
  {
    log::Info("GPU used: {}", vkbDevice.physical_device.name);
  }

  if (_debug_messenger)
  {
    log::Info("Vulkan debug output enabled.");
  }
  // Get the VkDevice handle used in the rest of a vulkan application
  _device = vkbDevice.device;

  volkLoadDevice(_device);

  _chosenGPU = physicalDevice.physical_device;

  // use vkbootstrap to get a Graphics queue
  _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
  _graphicsQueueFamily =
      vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

  // initialize the memory allocator
  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice = _chosenGPU;
  allocatorInfo.device = _device;
  allocatorInfo.instance = _instance;
  allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

  VmaVulkanFunctions vulkanFunctions = {};
  vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
  vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
  vulkanFunctions.vkAllocateMemory = vkAllocateMemory;
  vulkanFunctions.vkBindBufferMemory = vkBindBufferMemory;
  vulkanFunctions.vkBindImageMemory = vkBindImageMemory;
  vulkanFunctions.vkCreateBuffer = vkCreateBuffer;
  vulkanFunctions.vkCreateImage = vkCreateImage;
  vulkanFunctions.vkDestroyBuffer = vkDestroyBuffer;
  vulkanFunctions.vkDestroyImage = vkDestroyImage;
  vulkanFunctions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
  vulkanFunctions.vkFreeMemory = vkFreeMemory;
  vulkanFunctions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
  vulkanFunctions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
  vulkanFunctions.vkGetPhysicalDeviceMemoryProperties =
      vkGetPhysicalDeviceMemoryProperties;
  vulkanFunctions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
  vulkanFunctions.vkInvalidateMappedMemoryRanges =
      vkInvalidateMappedMemoryRanges;
  vulkanFunctions.vkMapMemory = vkMapMemory;
  vulkanFunctions.vkUnmapMemory = vkUnmapMemory;
  vulkanFunctions.vkCmdCopyBuffer = vkCmdCopyBuffer;

  allocatorInfo.pVulkanFunctions = &vulkanFunctions;
  vmaCreateAllocator(&allocatorInfo, &_allocator);

  _mainDeletionQueue.push_function(
      [&]()
      {
        vmaDestroyAllocator(_allocator);
      });
}
void internal::init_commands()
{
  // create a command pool for commands submitted to the graphics queue.
  // we also want the pool to allow for resetting of individual command
  // buffers
  VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(
      _graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  for (int i = 0; i < FRAME_OVERLAP; i++)
  {
    VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr,
                                 &_frames[i]._commandPool));

    // allocate the default command buffer that we will use for rendering
    VkCommandBufferAllocateInfo cmdAllocInfo =
        vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);

    VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo,
                                      &_frames[i]._mainCommandBuffer));
  }

  // immediate commands
  VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr,
                               &_immCommandPool));

  // allocate the command buffer for immediate submits
  VkCommandBufferAllocateInfo cmdAllocInfo =
      vkinit::command_buffer_allocate_info(_immCommandPool, 1);

  VK_CHECK(
      vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_immCommandBuffer));

  _mainDeletionQueue.push_function(
      [=]()
      {
        vkDestroyCommandPool(_device, _immCommandPool, nullptr);
      });
}
void internal::immediate_submit(
    std::function<void(VkCommandBuffer cmd)>&& function)
{
  VK_CHECK(vkResetFences(_device, 1, &_immFence));
  VK_CHECK(vkResetCommandBuffer(_immCommandBuffer, 0));

  VkCommandBuffer cmd = _immCommandBuffer;

  VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

  function(cmd);

  VK_CHECK(vkEndCommandBuffer(cmd));

  VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);
  VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, nullptr, nullptr);

  // submit command buffer to the queue and execute it.
  //  _renderFence will now block until the graphic commands finish execution
  VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, _immFence));

  VK_CHECK(vkWaitForFences(_device, 1, &_immFence, true, 9999999999));
}

void internal::init_sync_structures()
{
  // create syncronization structures
  // one fence to control when the gpu has finished rendering the frame,
  // and 2 semaphores to syncronize rendering with swapchain
  // we want the fence to start signalled so we can wait on it on the first
  // frame
  VkFenceCreateInfo fenceCreateInfo =
      vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
  VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

  for (int i = 0; i < FRAME_OVERLAP; i++)
  {
    VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr,
                           &_frames[i]._renderFence));

    VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr,
                               &_frames[i]._swapchainSemaphore));
    VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr,
                               &_frames[i]._renderSemaphore));
  }

  // immediate commands
  VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_immFence));
  _mainDeletionQueue.push_function(
      [=]()
      {
        vkDestroyFence(_device, _immFence, nullptr);
      });
}

void hm::Device::ResizeSwapchain()
{
  vkDeviceWaitIdle(_device);

  destroy_swapchain();

  int w, h;
  SDL_GetWindowSize(m_pWindow, &w, &h);
  m_windowSize.x = w;
  m_windowSize.y = h;

  create_swapchain(m_windowSize.x, m_windowSize.y);

  m_bResizeRequested = false;
}

void internal::create_swapchain(u32 width, u32 height)
{
  vkb::SwapchainBuilder swapchainBuilder {_chosenGPU, _device, _surface};

  _swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

  vkb::Swapchain vkbSwapchain =
      swapchainBuilder
          //.use_default_format_selection()
          .set_desired_format(VkSurfaceFormatKHR {
              .format = _swapchainImageFormat,
              .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
          // use vsync present mode
          .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
          .set_desired_extent(width, height)
          .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
          .build()
          .value();

  _swapchainExtent = vkbSwapchain.extent;
  // store swapchain and its related images
  _swapchain = vkbSwapchain.swapchain;
  _swapchainImages = vkbSwapchain.get_images().value();
  _swapchainImageViews = vkbSwapchain.get_image_views().value();
}
AllocatedBuffer hm::create_buffer(size_t allocSize, VkBufferUsageFlags usage,
                                  VmaMemoryUsage memoryUsage)
{
  // allocate buffer
  VkBufferCreateInfo bufferInfo = {.sType =
                                       VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.pNext = nullptr;
  bufferInfo.size = allocSize;

  bufferInfo.usage = usage;

  VmaAllocationCreateInfo vmaallocInfo = {};
  vmaallocInfo.usage = memoryUsage;
  vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
  AllocatedBuffer newBuffer;

  // allocate the buffer
  VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo,
                           &newBuffer.buffer, &newBuffer.allocation,
                           &newBuffer.info));

  return newBuffer;
}

void internal::destroy_swapchain()
{
  vkDestroySwapchainKHR(_device, _swapchain, nullptr);

  // destroy swapchain resources
  for (int i = 0; i < _swapchainImageViews.size(); i++)
  {
    vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
  }
}
void hm::destroy_buffer(const AllocatedBuffer& buffer)
{
  vmaDestroyBuffer(_allocator, buffer.buffer, buffer.allocation);
}

void internal::init_swapchain(glm::uvec2 windowSize)
{
  create_swapchain(windowSize.x, windowSize.y);

  // draw image size will match the window
  VkExtent3D drawImageExtent = {windowSize.x, windowSize.y, 1};

  // hardcoding the draw format to 32 bit float
  _drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
  _drawImage.imageExtent = drawImageExtent;

  VkImageUsageFlags drawImageUsages {};
  drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
  drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  VkImageCreateInfo rimg_info = vkinit::image_create_info(
      _drawImage.imageFormat, drawImageUsages, drawImageExtent);

  // for the draw image, we want to allocate it from gpu local memory
  VmaAllocationCreateInfo rimg_allocinfo = {};
  rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  rimg_allocinfo.requiredFlags =
      VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  // allocate and create the image
  vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_drawImage.image,
                 &_drawImage.allocation, nullptr);

  // build a image-view for the draw image to use for rendering
  VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(
      _drawImage.imageFormat, _drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

  VK_CHECK(
      vkCreateImageView(_device, &rview_info, nullptr, &_drawImage.imageView));

  _depthImage.imageFormat = VK_FORMAT_D32_SFLOAT;
  _depthImage.imageExtent = drawImageExtent;
  VkImageUsageFlags depthImageUsages {};
  depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

  VkImageCreateInfo dimg_info = vkinit::image_create_info(
      _depthImage.imageFormat, depthImageUsages, drawImageExtent);

  // allocate and create the image
  vmaCreateImage(_allocator, &dimg_info, &rimg_allocinfo, &_depthImage.image,
                 &_depthImage.allocation, nullptr);

  // build a image-view for the draw image to use for rendering
  VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(
      _depthImage.imageFormat, _depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

  VK_CHECK(
      vkCreateImageView(_device, &dview_info, nullptr, &_depthImage.imageView));
  // add to deletion queues
  _mainDeletionQueue.push_function(
      [=]()
      {
        vkDestroyImageView(_device, _drawImage.imageView, nullptr);
        vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);
        vkDestroyImageView(_device, _depthImage.imageView, nullptr);
        vmaDestroyImage(_allocator, _depthImage.image, _depthImage.allocation);
      });
}
