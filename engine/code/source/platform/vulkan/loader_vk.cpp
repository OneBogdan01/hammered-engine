
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
#include "platform/vulkan/device_vk.hpp"
using namespace tinygltf;
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
