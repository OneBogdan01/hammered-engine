#pragma once
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
  std::shared_ptr<GLTFMaterial> material;
};

struct MeshAsset
{
  std::string name;

  std::vector<GeoSurface> surfaces;
  GPUMeshBuffers meshBuffers;
};

std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(
     const std::filesystem::path&filePath);
// forward declaration

} // namespace hm
