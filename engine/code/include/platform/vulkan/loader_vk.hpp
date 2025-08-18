#pragma once
#include "descriptors_vk.hpp"

#include "types_vk.hpp"

#include <unordered_map>
#include <filesystem>

namespace hm
{
class Device;

struct GLTFMaterial
{
  MaterialInstance data;
};

struct GeoSurface
{
  uint32_t startIndex;
  uint32_t count;
  Bounds bounds;
  std::shared_ptr<GLTFMaterial> material;
};

struct MeshAsset
{
  std::string name;

  std::vector<GeoSurface> surfaces;
  GPUMeshBuffers meshBuffers;
};

std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(
    const std::filesystem::path& filePath);
// forward declaration

struct LoadedGLTF : public IRenderable
{
  // storage for all the data on a given glTF file
  std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
  std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
  std::unordered_map<std::string, AllocatedImage> images;
  std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> materials;

  // nodes that dont have a parent, for iterating through the file in tree order
  std::vector<std::shared_ptr<Node>> topNodes;

  std::vector<VkSampler> samplers;

  DescriptorAllocatorGrowable descriptorPool;

  AllocatedBuffer materialDataBuffer;

  // TODO use this instead
  //~LoadedGLTF() { clearAll(TODO); };
  void clearAll(VkDevice dv);

  virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx);

 private:
};
// forward declaration
std::optional<std::shared_ptr<hm::LoadedGLTF>> loadGltf(
    VkDevice _device, const std::filesystem::path& filePath);
} // namespace hm
