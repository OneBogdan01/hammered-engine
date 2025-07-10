#pragma once

#include <fmt/core.h>
#include <vk_mem_alloc.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>

#include <array>
#include <deque>
#include <functional>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

struct AllocatedImage
{
  VkImage image;
  VkImageView imageView;
  VmaAllocation allocation;
  VkExtent3D imageExtent;
  VkFormat imageFormat;
};

#define VK_CHECK(x)                                                          \
  do                                                                         \
  {                                                                          \
    VkResult err = x;                                                        \
    if (err)                                                                 \
    {                                                                        \
      fmt::println("Vulkan error at {}:{} - {}: {}", __FILE__, __LINE__, #x, \
                   string_VkResult(err));                                    \
      abort();                                                               \
    }                                                                        \
  } while (0)
