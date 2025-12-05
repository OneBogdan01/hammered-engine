
#include "platform/vulkan/loader_vk.hpp"
// TODO replace with ktx

#include "SDL3/SDL_assert.h"
#include "external/tracy_impl.hpp"

#include "utility/logger.hpp"
#include <stb_image.h>
#include <volk.h>
#include "platform/vulkan/device_vk.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

using namespace tinygltf;
using namespace hm;
inline void ReadComponent(const unsigned char* ptr, int type, int c, bool norm,
                          float& out)
{
  switch (type)
  {
    case TINYGLTF_COMPONENT_TYPE_FLOAT:
      out = ((float*)ptr)[c];
      break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
      out = norm ? ((uint8_t*)ptr)[c] / 255.f : float(((uint8_t*)ptr)[c]);
      break;
    case TINYGLTF_COMPONENT_TYPE_BYTE:
      out = norm ? std::max(-1.f, ((int8_t*)ptr)[c] / 127.f)
                 : float(((int8_t*)ptr)[c]);
      break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
      out = norm ? ((uint16_t*)ptr)[c] / 65535.f : float(((uint16_t*)ptr)[c]);
      break;
    case TINYGLTF_COMPONENT_TYPE_SHORT:
      out = norm ? std::max(-1.f, ((int16_t*)ptr)[c] / 32767.f)
                 : float(((int16_t*)ptr)[c]);
      break;
  }
}
std::optional<AllocatedImage> load_image(const tinygltf::Model& model,
                                         const tinygltf::Image& image)
{
  AllocatedImage newImage {};
  int width = 0, height = 0, channels = 0;

  auto upload_image = [&](unsigned char* pixels)
  {
    VkExtent3D imagesize {(uint32_t)width, (uint32_t)height, 1};
    newImage = create_image(pixels, imagesize, VK_FORMAT_R8G8B8A8_UNORM,
                            VK_IMAGE_USAGE_SAMPLED_BIT, true);
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
} // TODO this is super slow for now
std::optional<std::vector<std::shared_ptr<hm::MeshAsset>>> hm::loadGltfMeshes(
    const std::filesystem::path& filePath)
{
  HM_ZONE_SCOPED;
  HM_ZONE_TEXT(filePath.string().c_str(), filePath.string().size());

  log::Info("=== Loading GLTF: {} ===", filePath.string());

  Model model;
  TinyGLTF loader;
  std::string err;
  std::string warn;

  bool ret;
  {
    HM_ZONE_SCOPED_N("Load Binary File");
    auto start = std::chrono::high_resolution_clock::now();
    ret = loader.LoadBinaryFromFile(&model, &err, &warn, filePath.string());
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    log::Info("  Binary load: {} us", duration.count());
  }

  if (!warn.empty())
  {
    log::Warning("  GLTF warning: {}", warn);
  }

  if (!err.empty())
  {
    log::Error("  GLTF error: {}", err);
  }
  if (!ret)
  {
    log::Error("Failed to parse GLTF file: {}", filePath.string());
    return {};
  }

  log::Info("  Found {} mesh(es), {} buffer(s), {} accessor(s)",
            model.meshes.size(), model.buffers.size(), model.accessors.size());

  std::vector<std::shared_ptr<MeshAsset>> meshes;
  std::vector<uint32_t> indices;
  std::vector<Vertex> vertices;

  size_t totalVertices = 0;
  size_t totalIndices = 0;
  size_t totalPrimitives = 0;

  for (size_t meshIdx = 0; meshIdx < model.meshes.size(); ++meshIdx)
  {
    auto& mesh = model.meshes[meshIdx];
    HM_ZONE_SCOPED_N("Process Mesh");
    HM_ZONE_TEXT(mesh.name.c_str(), mesh.name.size());

    auto meshStart = std::chrono::high_resolution_clock::now();

    log::Info("  Processing mesh [{}]: '{}'", meshIdx, mesh.name);

    MeshAsset meshAsset;
    meshAsset.name = mesh.name;
    indices.clear();
    vertices.clear();

    size_t meshVertexCount = 0;
    size_t meshIndexCount = 0;

    for (size_t primIdx = 0; primIdx < mesh.primitives.size(); ++primIdx)
    {
      auto& primitive = mesh.primitives[primIdx];
      HM_ZONE_SCOPED_N("Process Primitive");

      GeoSurface newSurface;
      newSurface.startIndex = static_cast<uint32_t>(indices.size());
      newSurface.count =
          static_cast<uint32_t>(model.accessors[primitive.indices].count);

      size_t initialVtx = vertices.size();

      {
        HM_ZONE_SCOPED_N("Parse Indices");
        auto& indexAccessor = model.accessors[primitive.indices];
        const auto& view = model.bufferViews[indexAccessor.bufferView];
        const auto& buffer = model.buffers[view.buffer];

        size_t indexCount = indexAccessor.count;
        const auto* basePtr =
            buffer.data.data() + view.byteOffset + indexAccessor.byteOffset;

        indices.reserve(indices.size() + indexCount);

        auto parseIndices = [&]<typename T>(const T* data)
        {
          for (size_t i = 0; i < indexCount; i++)
          {
            indices.push_back(static_cast<uint32_t>(data[i]) + initialVtx);
            if (i % 100 == 0)
            {
              log::Debug("    Index[{}]: {}", i, indices.back());
            }
          }
        };

        switch (indexAccessor.componentType)
        {
          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            parseIndices(reinterpret_cast<const uint32_t*>(basePtr));
            break;
          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            parseIndices(reinterpret_cast<const uint16_t*>(basePtr));
            break;
          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            parseIndices(reinterpret_cast<const uint8_t*>(basePtr));
            break;
        }

        meshIndexCount += indexCount;
        HM_ZONE_VALUE(static_cast<int64_t>(indexCount));
      }

      size_t vertexCount = 0;
      {
        HM_ZONE_SCOPED_N("Parse Positions");
        auto& accessor =
            model.accessors[primitive.attributes.find("POSITION")->second];
        vertexCount = accessor.count;
        vertices.resize(vertices.size() + vertexCount);

        const auto& view = model.bufferViews[accessor.bufferView];
        const auto& buffer = model.buffers[view.buffer];
        Vertex vert {};
        vert.color = glm::vec4 {1.0f};
        const auto* data = reinterpret_cast<const float*>(
            &buffer.data[view.byteOffset + accessor.byteOffset]);
        for (size_t i = 0; i < vertexCount; i++)
        {
          vert.position.x = data[i * 3 + 0];
          vert.position.y = data[i * 3 + 1];
          vert.position.z = data[i * 3 + 2];
          vertices[initialVtx + i] = vert;
        }

        log::Debug("    Parsed {} positions (first: [{:.2f}, {:.2f}, {:.2f}])",
                   vertexCount, vertices[initialVtx].position.x,
                   vertices[initialVtx].position.y,
                   vertices[initialVtx].position.z);

        meshVertexCount += vertexCount;
        HM_ZONE_VALUE(static_cast<int64_t>(vertexCount));
      }

      {
        HM_ZONE_SCOPED_N("Parse Normals");
        auto iterator = primitive.attributes.find("NORMAL");
        if (iterator != primitive.attributes.end())
        {
          auto& accessor = model.accessors[iterator->second];
          const auto& view = model.bufferViews[accessor.bufferView];
          const auto& buffer = model.buffers[view.buffer];

          const auto* data = reinterpret_cast<const float*>(
              &buffer.data[view.byteOffset + accessor.byteOffset]);
          for (size_t i = 0; i < accessor.count; i++)
          {
            vertices[initialVtx + i].normal = {data[i * 3 + 0], data[i * 3 + 1],
                                               data[i * 3 + 2]};
          }

          log::Debug("    Parsed {} normals", accessor.count);
        }
      }

      {
        HM_ZONE_SCOPED_N("Parse UVs");
        auto iterator = primitive.attributes.find("TEXCOORD_0");
        if (iterator != primitive.attributes.end())
        {
          auto& accessor = model.accessors[iterator->second];
          const auto& view = model.bufferViews[accessor.bufferView];
          const auto& buffer = model.buffers[view.buffer];

          const auto* data = reinterpret_cast<const float*>(
              &buffer.data[view.byteOffset + accessor.byteOffset]);
          for (size_t i = 0; i < accessor.count; i++)
          {
            vertices[initialVtx + i].uv_x = data[i * 2 + 0];
            vertices[initialVtx + i].uv_y = data[i * 2 + 1];
          }

          log::Debug("    Parsed {} UVs", accessor.count);
        }
      }

      {
        HM_ZONE_SCOPED_N("Parse Colors");
        auto iterator = primitive.attributes.find("COLOR_0");
        if (iterator != primitive.attributes.end())
        {
          auto& accessor = model.accessors[iterator->second];
          const auto& view = model.bufferViews[accessor.bufferView];
          const auto& buffer = model.buffers[view.buffer];

          const auto* data = reinterpret_cast<const float*>(
              &buffer.data[view.byteOffset + accessor.byteOffset]);
          for (size_t i = 0; i < accessor.count; i++)
          {
            vertices[initialVtx + i].color = {data[i * 4 + 0], data[i * 4 + 1],
                                              data[i * 4 + 2], data[i * 4 + 3]};
          }

          log::Debug("    Parsed {} colors", accessor.count);
        }
      }

      log::Info("    Primitive [{}]: {} vertices, {} indices", primIdx,
                vertexCount, newSurface.count);

      meshAsset.surfaces.push_back(newSurface);
    }

    totalPrimitives += mesh.primitives.size();

    constexpr bool OverrideColors = false;
    if (OverrideColors)
    {
      for (Vertex& vtx : vertices)
      {
        vtx.color = glm::vec4(vtx.normal, 1.f);
      }
    }

    {
      HM_ZONE_SCOPED_N("Upload Mesh");
      auto uploadStart = std::chrono::high_resolution_clock::now();
      meshAsset.meshBuffers = UploadMesh(indices, vertices);
      auto uploadEnd = std::chrono::high_resolution_clock::now();
      auto uploadDuration =
          std::chrono::duration_cast<std::chrono::microseconds>(uploadEnd -
                                                                uploadStart);
      log::Info("    GPU upload: {} us", uploadDuration.count());
    }

    meshes.emplace_back(std::make_shared<MeshAsset>(std::move(meshAsset)));

    auto meshEnd = std::chrono::high_resolution_clock::now();
    auto meshDuration = std::chrono::duration_cast<std::chrono::microseconds>(
        meshEnd - meshStart);

    totalVertices += meshVertexCount;
    totalIndices += meshIndexCount;

    log::Info("    Mesh '{}': {} primitives, {} verts, {} indices ({} us)",
              mesh.name, mesh.primitives.size(), meshVertexCount,
              meshIndexCount, meshDuration.count());
  }

  log::Info("=== GLTF Load Summary ===");
  log::Info("  File: {}", filePath.filename().string());
  log::Info("  Meshes: {}", meshes.size());
  log::Info("  Primitives: {}", totalPrimitives);
  log::Info("  Total vertices: {}", totalVertices);
  log::Info("  Total indices: {}", totalIndices);
  log::Info("  Vertex buffer size: {:.2f} KB",
            (totalVertices * sizeof(Vertex)) / 1024.0f);
  log::Info("  Index buffer size: {:.2f} KB",
            (totalIndices * sizeof(uint32_t)) / 1024.0f);

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
    def.bounds = s.bounds;
    def.transform = nodeMatrix;
    def.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress;

    if (s.material->data.passType == MaterialPass::Transparent)
    {
      ctx.TransparentSurfaces.push_back(def);
    }
    else
    {
      ctx.OpaqueSurfaces.push_back(def);
    }
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
      // we failed to load, so lets give the slot a default white texture to
      // not completely break loading
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
      SDL_assert(tex.sampler >= 0);

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
        // get bounds of mesh
        glm::vec3 minpos(static_cast<float>(accessor.minValues[0]),
                         static_cast<float>(accessor.minValues[1]),
                         static_cast<float>(accessor.minValues[2]));

        glm::vec3 maxpos(static_cast<float>(accessor.maxValues[0]),
                         static_cast<float>(accessor.maxValues[1]),
                         static_cast<float>(accessor.maxValues[2]));

        newSurface.bounds.origin = (maxpos + minpos) * 0.5f;
        newSurface.bounds.extents = (maxpos - minpos) * 0.5f;
        newSurface.bounds.sphereRadius = glm::length(newSurface.bounds.extents);
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
      // TODO get color by vertex
      {
        auto it = primitive.attributes.find("COLOR_0");
        if (it != primitive.attributes.end())
        {
          const auto& accessor = model.accessors[it->second];
          const auto& view = model.bufferViews[accessor.bufferView];
          const auto& buffer = model.buffers[view.buffer];

          const unsigned char* base =
              buffer.data.data() + view.byteOffset + accessor.byteOffset;
          size_t stride = accessor.ByteStride(view);
          int numComp = tinygltf::GetNumComponentsInType(accessor.type);

          for (size_t i = 0; i < accessor.count; ++i)
          {
            glm::vec4 color(1.0f); // default white
            const unsigned char* ptr = base + i * stride;

            for (int c = 0; c < numComp; ++c)
            {
              float v = 0.0f;
              ReadComponent(ptr, accessor.componentType, c, accessor.normalized,
                            v);
              color[c] = v;
            }
            vertices[initialVtx + i].color = color;
          }
        }
      }
      if (primitive.material >= 0)
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
