﻿#pragma once

#include <fmt/format.h>
#include <vk_mem_alloc.h>

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
#include <cstdio>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace hm
{
struct MeshAsset;
}
namespace hm
{

/**
 * @brief Helper function to convert a data type
 *        to string using output stream operator.
 * @param value The object to be converted to string
 * @return String version of the given object
 */
template<class T>
inline std::string to_string(const T& value)
{
  std::stringstream ss;
  ss << std::fixed << value;
  return ss.str();
}

// https://github.com/KhronosGroup/Vulkan-Samples/blob/main/framework/common/vk_common.cpp#L27
inline std::ostream& operator<<(std::ostream& os, const VkResult result)
{
#define WRITE_VK_ENUM(r) \
  case VK_##r:           \
    os << #r;            \
    break;

  switch (result)
  {
    WRITE_VK_ENUM(NOT_READY);
    WRITE_VK_ENUM(TIMEOUT);
    WRITE_VK_ENUM(EVENT_SET);
    WRITE_VK_ENUM(EVENT_RESET);
    WRITE_VK_ENUM(INCOMPLETE);
    WRITE_VK_ENUM(ERROR_OUT_OF_HOST_MEMORY);
    WRITE_VK_ENUM(ERROR_OUT_OF_DEVICE_MEMORY);
    WRITE_VK_ENUM(ERROR_INITIALIZATION_FAILED);
    WRITE_VK_ENUM(ERROR_DEVICE_LOST);
    WRITE_VK_ENUM(ERROR_MEMORY_MAP_FAILED);
    WRITE_VK_ENUM(ERROR_LAYER_NOT_PRESENT);
    WRITE_VK_ENUM(ERROR_EXTENSION_NOT_PRESENT);
    WRITE_VK_ENUM(ERROR_FEATURE_NOT_PRESENT);
    WRITE_VK_ENUM(ERROR_INCOMPATIBLE_DRIVER);
    WRITE_VK_ENUM(ERROR_TOO_MANY_OBJECTS);
    WRITE_VK_ENUM(ERROR_FORMAT_NOT_SUPPORTED);
    WRITE_VK_ENUM(ERROR_SURFACE_LOST_KHR);
    WRITE_VK_ENUM(ERROR_NATIVE_WINDOW_IN_USE_KHR);
    WRITE_VK_ENUM(SUBOPTIMAL_KHR);
    WRITE_VK_ENUM(ERROR_OUT_OF_DATE_KHR);
    WRITE_VK_ENUM(ERROR_INCOMPATIBLE_DISPLAY_KHR);
    WRITE_VK_ENUM(ERROR_VALIDATION_FAILED_EXT);
    WRITE_VK_ENUM(ERROR_INVALID_SHADER_NV);
    default:
      os << "UNKNOWN_ERROR";
  }

#undef WRITE_VK_ENUM

  return os;
}

struct AllocatedImage
{
  VkImage image;
  VkImageView imageView;
  VmaAllocation allocation;
  VkExtent3D imageExtent;
  VkFormat imageFormat;
};

struct AllocatedBuffer
{
  // Vulkan handle
  VkBuffer buffer;
  // Used to release it at the end with VMA
  VmaAllocation allocation;
  VmaAllocationInfo info;
};
struct Vertex
{
  glm::vec3 position;
  float uv_x {0};
  glm::vec3 normal {1.0f, 0.0f, 0.0f};
  float uv_y {0};
  glm::vec4 color {1.0f};
};

// holds the resources needed for a mesh
struct GPUMeshBuffers
{
  AllocatedBuffer indexBuffer;
  AllocatedBuffer vertexBuffer;
  VkDeviceAddress vertexBufferAddress;
};

// push constants for our mesh object draws
struct GPUDrawPushConstants
{
  glm::mat4 worldMatrix;
  VkDeviceAddress vertexBuffer;
};
enum class MaterialPass : uint8_t
{
  MainColor,
  Transparent,
  Other
};
struct MaterialPipeline
{
  VkPipeline pipeline;
  VkPipelineLayout layout;
};

struct MaterialInstance
{
  MaterialPipeline* pipeline;
  VkDescriptorSet materialSet;
  MaterialPass passType;
};
struct DrawContext;

// base class for a renderable dynamic object
class IRenderable
{
  virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx) = 0;
};

// implementation of a drawable scene node.
// the scene node can hold children and will also keep a transform to propagate
// to them
struct Node : public IRenderable
{
  // parent pointer must be a weak pointer to avoid circular dependencies
  std::weak_ptr<Node> parent;
  std::vector<std::shared_ptr<Node>> children;

  glm::mat4 localTransform;
  glm::mat4 worldTransform;

  void refreshTransform(const glm::mat4& parentMatrix)
  {
    worldTransform = parentMatrix * localTransform;
    for (auto c : children)
    {
      c->refreshTransform(worldTransform);
    }
  }

  virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx)
  {
    // draw children
    for (auto& c : children)
    {
      c->Draw(topMatrix, ctx);
    }
  }
};
struct Bounds
{
  glm::vec3 origin;
  float sphereRadius;
  glm::vec3 extents;
};
struct RenderObject
{
  uint32_t indexCount;
  uint32_t firstIndex;
  VkBuffer indexBuffer;

  MaterialInstance* material;
  Bounds bounds;
  glm::mat4 transform;
  VkDeviceAddress vertexBufferAddress;
};
struct DrawContext
{
  std::vector<RenderObject> OpaqueSurfaces {};
  std::vector<RenderObject> TransparentSurfaces;
};
struct MeshNode : public Node
{
  std::shared_ptr<hm::MeshAsset> mesh;

  virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx) override;
};

#define VK_CHECK(x)                                                         \
  do                                                                        \
  {                                                                         \
    VkResult err = x;                                                       \
    if (err)                                                                \
    {                                                                       \
      throw std::runtime_error("Detected Vulkan error: " + to_string(err)); \
    }                                                                       \
  } while (0)

} // namespace hm
