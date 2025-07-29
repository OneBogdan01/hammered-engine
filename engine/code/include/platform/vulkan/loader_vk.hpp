#pragma once
#include "types_vk.hpp"

#include <unordered_map>
#include <filesystem>

namespace hm
{
class Device;

struct GeoSurface
{
  uint32_t startIndex;
  uint32_t count;
};

struct MeshAsset
{
  std::string name;

  std::vector<GeoSurface> surfaces;
  GPUMeshBuffers meshBuffers;
};
std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(
    Device* engine, std::filesystem::path filePath);
// forward declaration

} // namespace hm
