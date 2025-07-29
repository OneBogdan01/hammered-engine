
#include "platform/vulkan/loader_vk.hpp"
// TODO replace with ktx
#include "stb_image.h"
#include <iostream>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "utility/console.hpp"
#include <iostream>
#include "tiny_gltf.h"

using namespace tinygltf;
std::optional<std::vector<std::shared_ptr<hm::MeshAsset>>> hm::loadGltfMeshes(
    Device* engine, const std::filesystem::path& filePath)
{
  log::Info("Loading GLTF: {}", filePath.string());

  Model model;
  TinyGLTF loader;
  std::string err;
  std::string warn;
  bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filePath.string());

  if (!warn.empty())
  {
    log::Info("{}", warn.c_str());
  }

  if (!err.empty())
  {
    log::Error("{}", err.c_str());
  }
  if (ret == false)
  {
    log::Error("Failed to parse GLTF file: {}", filePath.string());
    return {};
  }
  std::vector<std::shared_ptr<MeshAsset>> meshes;
  std::vector<uint32_t> indices;
  std::vector<Vertex> vertices;
  for (auto& mesh : model.meshes)
  {
    MeshAsset meshAsset;
    meshAsset.name = mesh.name;
    indices.clear();
    vertices.clear();
  }
}
