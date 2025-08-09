
#include "platform/vulkan/loader_vk.hpp"
// TODO replace with ktx
#include "stb_image.h"
#include <iostream>

#define TINYGLTF_IMPLEMENTATION
// TODO replace with ktx
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include "utility/console.hpp"
#include <iostream>

#include <volk.h>
#include "platform/vulkan/device_vk.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
using namespace tinygltf;
using namespace hm;
std::optional<AllocatedImage> load_image(const tinygltf::Model& model,
                                         const tinygltf::Image& image)
{
  AllocatedImage newImage {};
  int width = 0, height = 0, channels = 0;

  auto upload_image = [&](unsigned char* pixels)
  {
    VkExtent3D imagesize {(uint32_t)width, (uint32_t)height, 1};
    newImage = create_image(pixels, imagesize, VK_FORMAT_R8G8B8A8_UNORM,
                            VK_IMAGE_USAGE_SAMPLED_BIT, false);
  };

  if (image.uri.empty())
  {
    if (image.bufferView >= 0)
    {
      const auto& view = model.bufferViews[image.bufferView];
      const auto& buffer = model.buffers[view.buffer];
      const unsigned char* ptr = buffer.data.data() + view.byteOffset;

      unsigned char* data =
          stbi_load_from_memory(ptr, static_cast<int>(view.byteLength), &width,
                                &height, &channels, 4);
      if (data)
      {
        upload_image(data);
        stbi_image_free(data);
      }
    }
    else if (!image.image.empty())
    {
      width = image.width;
      height = image.height;
      channels = image.component;

      // Make a mutable copy since Vulkan upload may need non-const data
      std::vector<unsigned char> mutableImage(image.image.begin(),
                                              image.image.end());
      upload_image(mutableImage.data());
    }
  }
  else
  {
    std::string path = image.uri;

    // Load file into memory
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (file)
    {
      std::streamsize size = file.tellg();
      file.seekg(0, std::ios::beg);
      std::vector<unsigned char> buffer(size);
      if (file.read(reinterpret_cast<char*>(buffer.data()), size))
      {
        unsigned char* data = stbi_load_from_memory(
            buffer.data(), static_cast<int>(buffer.size()), &width, &height,
            &channels, 4);
        if (data)
        {
          upload_image(data);
          stbi_image_free(data);
        }
      }
    }
  }

  if (newImage.image == VK_NULL_HANDLE)
    return {};
  return newImage;
}

VkFilter extract_filter(int32_t filter)
{
  switch (filter)
  {
    // nearest samplers
    case TINYGLTF_TEXTURE_FILTER_NEAREST:
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
      return VK_FILTER_NEAREST;

    // linear samplers
    case TINYGLTF_TEXTURE_FILTER_LINEAR:
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:

    default:
      return VK_FILTER_LINEAR;
  }
}

VkSamplerMipmapMode extract_mipmap_mode(int32_t filter)
{
  switch (filter)
  {
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
      return VK_SAMPLER_MIPMAP_MODE_NEAREST;

    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
    default:
      return VK_SAMPLER_MIPMAP_MODE_LINEAR;
  }
}
// TODO this is super slow for now
std::optional<std::vector<std::shared_ptr<hm::MeshAsset>>> hm::loadGltfMeshes(
    const std::filesystem::path& filePath)
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
    for (auto& primitive : mesh.primitives)
    {
      GeoSurface newSurface;
      newSurface.startIndex = static_cast<uint32_t>(indices.size());
      newSurface.count =
          static_cast<uint32_t>(model.accessors[primitive.indices].count);

      size_t initialVtx = vertices.size();

      {
        auto& indexAccessor = model.accessors[primitive.indices];
        const auto& view = model.bufferViews[indexAccessor.bufferView];
        const auto& buffer = model.buffers[view.buffer];
        if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
        {
          const auto* data = reinterpret_cast<const uint32_t*>(
              buffer.data.data() + view.byteOffset + indexAccessor.byteOffset);
          for (size_t i = 0; i < indexAccessor.count; i++)
            indices.push_back(static_cast<uint32_t>(data[i]) + initialVtx);
        }
        else if (indexAccessor.componentType ==
                 TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
        {
          const auto* data = reinterpret_cast<const uint16_t*>(
              buffer.data.data() + view.byteOffset + indexAccessor.byteOffset);
          for (size_t i = 0; i < indexAccessor.count; i++)
            indices.push_back(static_cast<uint32_t>(data[i]) + initialVtx);
        }
        else if (indexAccessor.componentType ==
                 TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
        {
          const auto* data =
              buffer.data.data() + view.byteOffset + indexAccessor.byteOffset;
          for (size_t i = 0; i < indexAccessor.count; i++)
            indices.push_back(static_cast<uint32_t>(data[i]) + initialVtx);
        }
      }

      {
        auto& accessor =
            model.accessors[primitive.attributes.find("POSITION")->second];
        vertices.resize(vertices.size() + accessor.count);

        const auto& view = model.bufferViews[accessor.bufferView];
        const auto& buffer = model.buffers[view.buffer];
        Vertex vert {};
        const auto* data = reinterpret_cast<const float*>(
            &buffer.data[view.byteOffset + accessor.byteOffset]);
        size_t index {0};
        for (; index < accessor.count; index++)
        {
          vert.position.x = data[index * 3 + 0];
          vert.position.y = data[index * 3 + 1];
          vert.position.z = data[index * 3 + 2];
          vertices[initialVtx + index] = vert;
        }
      }
      {
        auto iterator = primitive.attributes.find("NORMAL");
        if (iterator != primitive.attributes.end())
        {
          auto& accessor = model.accessors[iterator->second];

          const auto& view = model.bufferViews[accessor.bufferView];
          const auto& buffer = model.buffers[view.buffer];

          const auto* data = reinterpret_cast<const float*>(
              &buffer.data[view.byteOffset + accessor.byteOffset]);
          size_t index {0};
          for (; index < accessor.count; index++)
          {
            glm::vec3 normal {};
            normal.x = data[index * 3 + 0];
            normal.y = data[index * 3 + 1];
            normal.z = data[index * 3 + 2];
            vertices[initialVtx + index].normal = normal;
          }
        }
      }
      {
        auto iterator = primitive.attributes.find("TEXCOORD_0");
        if (iterator != primitive.attributes.end())
        {
          auto& accessor = model.accessors[iterator->second];

          const auto& view = model.bufferViews[accessor.bufferView];
          const auto& buffer = model.buffers[view.buffer];

          const auto* data = reinterpret_cast<const float*>(
              &buffer.data[view.byteOffset + accessor.byteOffset]);
          size_t index {0};
          for (; index < accessor.count; index++)
          {
            glm::vec2 uv {};
            uv.x = data[index * 2 + 0];
            uv.y = data[index * 2 + 1];
            vertices[initialVtx + index].uv_x = uv.x;
            vertices[initialVtx + index].uv_y = uv.y;
          }
        }
      }
      {
        auto iterator = primitive.attributes.find("COLOR_0");
        if (iterator != primitive.attributes.end())
        {
          auto& accessor = model.accessors[iterator->second];

          const auto& view = model.bufferViews[accessor.bufferView];
          const auto& buffer = model.buffers[view.buffer];

          const auto* data = reinterpret_cast<const float*>(
              &buffer.data[view.byteOffset + accessor.byteOffset]);
          size_t index {0};
          for (; index < accessor.count; index++)
          {
            glm::vec4 color {};
            color.r = data[index * 4 + 0];
            color.g = data[index * 4 + 1];
            color.b = data[index * 4 + 2];
            color.a = data[index * 4 + 3];
            vertices[initialVtx + index].color = color;
          }
        }
      }
      meshAsset.surfaces.push_back(newSurface);
    }
    constexpr bool OverrideColors = false;
    if (OverrideColors)
    {
      for (Vertex& vtx : vertices)
      {
        vtx.color = glm::vec4(vtx.normal, 1.f);
      }
    }
    meshAsset.meshBuffers = UploadMesh(indices, vertices);

    meshes.emplace_back(std::make_shared<MeshAsset>(std::move(meshAsset)));
  }

  return meshes;
}

void MeshNode::Draw(const glm::mat4& topMatrix, DrawContext& ctx)
{
  glm::mat4 nodeMatrix = topMatrix * worldTransform;

  for (auto& s : mesh->surfaces)
  {
    RenderObject def;
    def.indexCount = s.count;
    def.firstIndex = s.startIndex;
    def.indexBuffer = mesh->meshBuffers.indexBuffer.buffer;
    def.material = &s.material->data;

    def.transform = nodeMatrix;
    def.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress;

    ctx.OpaqueSurfaces.push_back(def);
  }

  // recurse down
  Node::Draw(topMatrix, ctx);
}

// TODO only works for vulkan
std::optional<std::shared_ptr<hm::LoadedGLTF>> hm::loadGltf(
    VkDevice _device, const std::filesystem::path& filePath)
{
  log::Info("Loading GLTF: {}", filePath.string());

  Model model;
  TinyGLTF loader;
  std::string err;
  std::string warn;
  bool res {false};

  if (filePath.string().ends_with(".gltf"))
  {
    res = loader.LoadASCIIFromFile(&model, &err, &warn, filePath.string());
  }
  else if (filePath.string().ends_with(".glb"))
  {
    res = loader.LoadBinaryFromFile(&model, &err, &warn, filePath.string());
  }
  if (!warn.empty())
  {
    log::Info("{}", warn.c_str());
  }

  if (!err.empty())
  {
    log::Error("{}", err.c_str());
  }
  if (res == false)
  {
    log::Error("Failed to parse GLTF file: {}", filePath.string());
    return {};
  }
  std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}};
  std::shared_ptr<LoadedGLTF> scene = std::make_shared<LoadedGLTF>();
  LoadedGLTF& file = *scene.get();

  file.descriptorPool.init(_device, model.materials.size(), sizes);
  // load samplers
  for (auto& sampler : model.samplers)
  {
    VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                                 .pNext = nullptr};
    sampl.maxLod = VK_LOD_CLAMP_NONE;
    sampl.minLod = 0;

    sampl.magFilter = extract_filter(sampler.magFilter);
    sampl.minFilter = extract_filter(sampler.minFilter);

    sampl.mipmapMode = extract_mipmap_mode(sampler.minFilter);

    VkSampler newSampler;
    vkCreateSampler(_device, &sampl, nullptr, &newSampler);

    file.samplers.push_back(newSampler);
  }

  // temporal arrays for all the objects to use while creating the GLTF data
  std::vector<std::shared_ptr<MeshAsset>> meshes;
  std::vector<std::shared_ptr<hm::Node>> nodes;
  std::vector<AllocatedImage> images;
  std::vector<std::shared_ptr<GLTFMaterial>> materials;

  // load all textures
  for (auto& image : model.images)
  {
    std::optional<AllocatedImage> img = load_image(model, image);

    if (img.has_value())
    {
      images.push_back(*img);
      file.images[image.name.c_str()] = *img;
    }
    else
    {
      // we failed to load, so lets give the slot a default white texture to not
      // completely break loading
      images.push_back(_errorCheckerboardImage);
      log::Error("Gltf failed to load texture {}", image.name);
    }
  }
  // create buffer to hold the material data
  file.materialDataBuffer = create_buffer(
      sizeof(GLTFMetallic_Roughness::MaterialConstants) *
          model.materials.size(),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
  int data_index = 0;
  GLTFMetallic_Roughness::MaterialConstants* sceneMaterialConstants =
      (GLTFMetallic_Roughness::MaterialConstants*)
          file.materialDataBuffer.info.pMappedData;

  // materials
  for (auto& mat : model.materials)
  {
    std::shared_ptr<GLTFMaterial> newMat = std::make_shared<GLTFMaterial>();
    materials.push_back(newMat);
    file.materials[mat.name.c_str()] = newMat;

    GLTFMetallic_Roughness::MaterialConstants constants;
    constants.colorFactors.x = mat.pbrMetallicRoughness.baseColorFactor[0];
    constants.colorFactors.y = mat.pbrMetallicRoughness.baseColorFactor[1];
    constants.colorFactors.z = mat.pbrMetallicRoughness.baseColorFactor[2];
    constants.colorFactors.w = mat.pbrMetallicRoughness.baseColorFactor[3];

    constants.metal_rough_factors.x = mat.pbrMetallicRoughness.metallicFactor;
    constants.metal_rough_factors.y = mat.pbrMetallicRoughness.roughnessFactor;
    // write material parameters to buffer
    sceneMaterialConstants[data_index] = constants;

    MaterialPass passType = MaterialPass::MainColor;
    if (mat.alphaMode == "BLEND")
    {
      passType = MaterialPass::Transparent;
    }

    GLTFMetallic_Roughness::MaterialResources materialResources;
    // default the material textures
    materialResources.colorImage = _whiteImage;
    materialResources.colorSampler = _defaultSamplerLinear;
    materialResources.metalRoughImage = _whiteImage;
    materialResources.metalRoughSampler = _defaultSamplerLinear;

    // set the uniform buffer for the material data
    materialResources.dataBuffer = file.materialDataBuffer.buffer;
    materialResources.dataBufferOffset =
        data_index * sizeof(GLTFMetallic_Roughness::MaterialConstants);
    // grab textures from gltf file
    if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0)
    {
      const Texture& tex =
          model.textures[mat.pbrMetallicRoughness.baseColorTexture.index];

      size_t img = tex.source;
      size_t sampler = tex.sampler;

      materialResources.colorImage = images[img];
      materialResources.colorSampler = file.samplers[sampler];
    }

    newMat->data = metalRoughMaterial.write_material(
        _device, passType, materialResources, file.descriptorPool);

    data_index++;
  }

  std::vector<uint32_t> indices;
  std::vector<Vertex> vertices;
  for (auto& mesh : model.meshes)
  {
    std::shared_ptr<MeshAsset> newmesh = std::make_shared<MeshAsset>();
    meshes.push_back(newmesh);
    file.meshes[mesh.name.c_str()] = newmesh;
    newmesh->name = mesh.name;
    indices.clear();
    vertices.clear();
    for (auto& primitive : mesh.primitives)
    {
      GeoSurface newSurface;
      newSurface.startIndex = static_cast<uint32_t>(indices.size());
      newSurface.count =
          static_cast<uint32_t>(model.accessors[primitive.indices].count);

      size_t initialVtx = vertices.size();

      {
        auto& indexAccessor = model.accessors[primitive.indices];
        const auto& view = model.bufferViews[indexAccessor.bufferView];
        const auto& buffer = model.buffers[view.buffer];
        if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
        {
          const auto* data = reinterpret_cast<const uint32_t*>(
              buffer.data.data() + view.byteOffset + indexAccessor.byteOffset);
          for (size_t i = 0; i < indexAccessor.count; i++)
            indices.push_back(static_cast<uint32_t>(data[i]) + initialVtx);
        }
        else if (indexAccessor.componentType ==
                 TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
        {
          const auto* data = reinterpret_cast<const uint16_t*>(
              buffer.data.data() + view.byteOffset + indexAccessor.byteOffset);
          for (size_t i = 0; i < indexAccessor.count; i++)
            indices.push_back(static_cast<uint32_t>(data[i]) + initialVtx);
        }
        else if (indexAccessor.componentType ==
                 TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
        {
          const auto* data =
              buffer.data.data() + view.byteOffset + indexAccessor.byteOffset;
          for (size_t i = 0; i < indexAccessor.count; i++)
            indices.push_back(static_cast<uint32_t>(data[i]) + initialVtx);
        }
      }

      {
        auto& accessor =
            model.accessors[primitive.attributes.find("POSITION")->second];
        vertices.resize(vertices.size() + accessor.count);

        const auto& view = model.bufferViews[accessor.bufferView];
        const auto& buffer = model.buffers[view.buffer];
        Vertex vert {};
        const auto* data = reinterpret_cast<const float*>(
            &buffer.data[view.byteOffset + accessor.byteOffset]);
        size_t index {0};
        for (; index < accessor.count; index++)
        {
          vert.position.x = data[index * 3 + 0];
          vert.position.y = data[index * 3 + 1];
          vert.position.z = data[index * 3 + 2];
          vertices[initialVtx + index] = vert;
        }
      }
      {
        auto iterator = primitive.attributes.find("NORMAL");
        if (iterator != primitive.attributes.end())
        {
          auto& accessor = model.accessors[iterator->second];

          const auto& view = model.bufferViews[accessor.bufferView];
          const auto& buffer = model.buffers[view.buffer];

          const auto* data = reinterpret_cast<const float*>(
              &buffer.data[view.byteOffset + accessor.byteOffset]);
          size_t index {0};
          for (; index < accessor.count; index++)
          {
            glm::vec3 normal {};
            normal.x = data[index * 3 + 0];
            normal.y = data[index * 3 + 1];
            normal.z = data[index * 3 + 2];
            vertices[initialVtx + index].normal = normal;
          }
        }
      }
      {
        auto iterator = primitive.attributes.find("TEXCOORD_0");
        if (iterator != primitive.attributes.end())
        {
          auto& accessor = model.accessors[iterator->second];

          const auto& view = model.bufferViews[accessor.bufferView];
          const auto& buffer = model.buffers[view.buffer];

          const auto* data = reinterpret_cast<const float*>(
              &buffer.data[view.byteOffset + accessor.byteOffset]);
          size_t index {0};
          for (; index < accessor.count; index++)
          {
            glm::vec2 uv {};
            uv.x = data[index * 2 + 0];
            uv.y = data[index * 2 + 1];
            vertices[initialVtx + index].uv_x = uv.x;
            vertices[initialVtx + index].uv_y = uv.y;
          }
        }
      }
      {
        auto iterator = primitive.attributes.find("COLOR_0");
        if (iterator != primitive.attributes.end())
        {
          auto& accessor = model.accessors[iterator->second];

          const auto& view = model.bufferViews[accessor.bufferView];
          const auto& buffer = model.buffers[view.buffer];

          const auto* data = reinterpret_cast<const float*>(
              &buffer.data[view.byteOffset + accessor.byteOffset]);
          size_t index {0};
          for (; index < accessor.count; index++)
          {
            glm::vec4 color {};
            color.r = data[index * 4 + 0];
            color.g = data[index * 4 + 1];
            color.b = data[index * 4 + 2];
            color.a = data[index * 4 + 3];
            vertices[initialVtx + index].color = color;
          }
        }
      }
      if (primitive.material > 0)
      {
        newSurface.material = materials[primitive.material];
      }
      else
      {
        newSurface.material = materials[0];
      }
      newmesh->surfaces.push_back(newSurface);
    }

    newmesh->meshBuffers = UploadMesh(indices, vertices);
  }
  for (tinygltf::Node& node : model.nodes)
  {
    std::shared_ptr<hm::Node> newNode;

    // Check if node has a mesh
    if (node.mesh >= 0)
    {
      newNode = std::make_shared<MeshNode>();
      static_cast<MeshNode*>(newNode.get())->mesh = meshes[node.mesh];
    }
    else
    {
      newNode = std::make_shared<hm::Node>();
    }

    nodes.push_back(newNode);
    file.nodes[node.name.c_str()];

    // Handle transform
    if (!node.matrix.empty() && node.matrix.size() == 16)
    {
      // Direct matrix
      glm::mat4 mat = glm::make_mat4(node.matrix.data());
      newNode->localTransform = mat;
    }
    else
    {
      // TRS fallback
      glm::vec3 tl = node.translation.empty()
                         ? glm::vec3(0.0f)
                         : glm::vec3(node.translation[0], node.translation[1],
                                     node.translation[2]);

      glm::quat rot = node.rotation.empty()
                          ? glm::quat(1, 0, 0, 0)
                          : glm::quat(node.rotation[3], node.rotation[0],
                                      node.rotation[1], node.rotation[2]);

      glm::vec3 sc =
          node.scale.empty()
              ? glm::vec3(1.0f)
              : glm::vec3(node.scale[0], node.scale[1], node.scale[2]);

      glm::mat4 tm = glm::translate(glm::mat4(1.0f), tl);
      glm::mat4 rm = glm::toMat4(rot);
      glm::mat4 sm = glm::scale(glm::mat4(1.0f), sc);

      newNode->localTransform = tm * rm * sm;
    }
  }
  // run loop again to setup transform hierarchy
  for (int i = 0; i < model.nodes.size(); i++)
  {
    auto& node = model.nodes[i];
    std::shared_ptr<hm::Node>& sceneNode = nodes[i];

    for (auto& c : node.children)
    {
      sceneNode->children.push_back(nodes[c]);
      nodes[c]->parent = sceneNode;
    }
  }

  // find the top nodes, with no parents
  for (auto& node : nodes)
  {
    if (node->parent.lock() == nullptr)
    {
      file.topNodes.push_back(node);
      node->refreshTransform(glm::mat4 {1.f});
    }
  }
  return scene;
}
void LoadedGLTF::Draw(const glm::mat4& topMatrix, DrawContext& ctx)
{
  // create renderables from the scenenodes
  for (auto& n : topNodes)
  {
    n->Draw(topMatrix, ctx);
  }
}
void LoadedGLTF::clearAll(VkDevice dv)
{
  descriptorPool.destroy_pools(dv);
  destroy_buffer(materialDataBuffer);

  for (auto& [k, v] : meshes)
  {
    destroy_buffer(v->meshBuffers.indexBuffer);
    destroy_buffer(v->meshBuffers.vertexBuffer);
  }

  for (auto& [k, v] : images)
  {
    if (v.image == _errorCheckerboardImage.image)
    {
      // dont destroy the default images
      continue;
    }
    destroy_image(v);
  }

  for (auto& sampler : samplers)
  {
    vkDestroySampler(dv, sampler, nullptr);
  }
}
