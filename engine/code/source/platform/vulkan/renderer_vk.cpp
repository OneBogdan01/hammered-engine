#include "core/renderer.hpp"

#include <volk.h>

#include "platform/vulkan/device_vk.hpp"
#define VMA_IMPLEMENTATION
#include "Common.h"

#include <vk_mem_alloc.h>

#include "camera.hpp"
#include "engine.hpp"
#include "core/device.hpp"
#include "core/fileio.hpp"
#include "external/imgui_impl.hpp"
#include "glslang/Public/ShaderLang.h"
#include "platform/vulkan/images_vk.hpp"
#include "platform/vulkan/initializers_vk.hpp"
#include "platform/vulkan/loader_vk.hpp"
#include "platform/vulkan/pipelines_vk.hpp"

namespace hm::internal
{
bool is_visible(const RenderObject& obj, const glm::mat4& viewproj)
{
  std::array<glm::vec3, 8> corners {
      glm::vec3 {1, 1, 1},   glm::vec3 {1, 1, -1},   glm::vec3 {1, -1, 1},
      glm::vec3 {1, -1, -1}, glm::vec3 {-1, 1, 1},   glm::vec3 {-1, 1, -1},
      glm::vec3 {-1, -1, 1}, glm::vec3 {-1, -1, -1},
  };

  glm::mat4 matrix = viewproj * obj.transform;

  glm::vec3 min = {1.5, 1.5, 1.5};
  glm::vec3 max = {-1.5, -1.5, -1.5};

  for (int c = 0; c < 8; c++)
  {
    // project each corner into clip space
    glm::vec4 v =
        matrix *
        glm::vec4(obj.bounds.origin + (corners[c] * obj.bounds.extents), 1.f);

    // perspective correction
    v.x = v.x / v.w;
    v.y = v.y / v.w;
    v.z = v.z / v.w;

    min = glm::min(glm::vec3 {v.x, v.y, v.z}, min);
    max = glm::max(glm::vec3 {v.x, v.y, v.z}, max);
  }

  // check the clip space box is within the view
  if (min.z > 1.f || max.z < 0.f || min.x > 1.f || max.x < -1.f ||
      min.y > 1.f || max.y < -1.f)
  {
    return false;
  }
  return true;
}
DescriptorAllocatorGrowable globalDescriptorAllocator;

VkDescriptorSet _drawImageDescriptors;
VkDescriptorSetLayout _drawImageDescriptorLayout;

VkPipeline _gradientPipeline;
VkPipelineLayout _gradientPipelineLayout;
VkPipelineLayout _trianglePipelineLayout;
VkPipeline _trianglePipeline;

void init_triangle_pipeline();
void init_pipelines();
void init_background_pipelines();
void init_descriptors();

void init_commands();
void init_sync_structures();
void init_default_data();

VkPipelineLayout _meshPipelineLayout;
VkPipeline _meshPipeline;

GPUMeshBuffers rectangle;

void init_mesh_pipeline();

// shuts down the engine
void cleanup();
void draw_background(VkCommandBuffer cmd);

void draw_geometry(VkCommandBuffer cmd);
std::vector<ComputeEffect> backgroundEffects;
int currentBackgroundEffect {0};

// TODO move
std::vector<std::shared_ptr<MeshAsset>> testMeshes;

float renderScale = 1.f;

GPUSceneData sceneData;

VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;

MaterialInstance defaultData;

DrawContext mainDrawContext;
std::unordered_map<std::string, std::shared_ptr<Node>> loadedNodes;

void update_scene(Camera& mainCamera);

std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> loadedScenes;
} // namespace hm::internal
using namespace hm::internal;
using namespace hm;
hm::gpx::Renderer::Renderer(const std::string& name) : System(name)
{
  init_descriptors();
  init_pipelines();
  init_default_data();
  auto& mainCamera = Engine::Instance().GetECS().GetSystem<Camera>();

  mainCamera.velocity = glm::vec3(0.f);
  mainCamera.position = glm::vec3(0, 0, 5);

  mainCamera.pitch = 0;
  mainCamera.yaw = 0;
}
gpx::Renderer::~Renderer()
{
  // make sure the gpu has stopped doing its things
  vkDeviceWaitIdle(_device);
  for (auto& scene : loadedScenes)
  {
    scene.second->clearAll(_device);
  }
  for (auto& mesh : testMeshes)
  {
    destroy_buffer(mesh->meshBuffers.indexBuffer);
    destroy_buffer(mesh->meshBuffers.vertexBuffer);
  }
  loadedScenes.clear();
}
void internal::init_pipelines()
{
  init_background_pipelines();
  init_triangle_pipeline();
  init_mesh_pipeline();
  metalRoughMaterial.build_pipelines();
}

void internal::init_background_pipelines()
{
  VkPipelineLayoutCreateInfo computeLayout {};
  computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  computeLayout.pNext = nullptr;
  computeLayout.pSetLayouts = &_drawImageDescriptorLayout;
  computeLayout.setLayoutCount = 1;

  VkPushConstantRange pushConstant {};
  pushConstant.offset = 0;
  pushConstant.size = sizeof(ComputePushConstants);
  pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  computeLayout.pPushConstantRanges = &pushConstant;
  computeLayout.pushConstantRangeCount = 1;

  VK_CHECK(vkCreatePipelineLayout(_device, &computeLayout, nullptr,
                                  &_gradientPipelineLayout));

  glslang::InitializeProcess();

  hm::vk::CompilerShaderModule(
      io::GetPath("shaders/gradient_color.comp").c_str(),
      io::GetPath("shaders/gradient_color.comp.vk.spv").c_str());
  VkShaderModule gradientShader;
  if (!vkutil::load_shader_module(
          io::GetPath("shaders/gradient_color.comp.vk.spv").c_str(), _device,
          &gradientShader))
  {
    log::Error("Error when building the compute shader \n");
  }

  VkShaderModule skyShader;
  if (!vkutil::load_shader_module(
          io::GetPath("shaders/sky.comp.vk.spv").c_str(), _device, &skyShader))
  {
    log::Error("Error when building the compute shader \n");
  }
  glslang::FinalizeProcess();
  VkPipelineShaderStageCreateInfo stageinfo {};
  stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stageinfo.pNext = nullptr;
  stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  stageinfo.module = gradientShader;
  stageinfo.pName = "main";

  VkComputePipelineCreateInfo computePipelineCreateInfo {};
  computePipelineCreateInfo.sType =
      VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineCreateInfo.pNext = nullptr;
  computePipelineCreateInfo.layout = _gradientPipelineLayout;
  computePipelineCreateInfo.stage = stageinfo;

  ComputeEffect gradient;
  gradient.layout = _gradientPipelineLayout;
  gradient.name = "gradient";
  gradient.data = {};

  // default colors
  gradient.data.data1 = glm::vec4(1, 0, 0, 1);
  gradient.data.data2 = glm::vec4(0, 0, 1, 1);

  VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1,
                                    &computePipelineCreateInfo, nullptr,
                                    &gradient.pipeline));

  // change the shader module only to create the sky shader
  computePipelineCreateInfo.stage.module = skyShader;

  ComputeEffect sky;
  sky.layout = _gradientPipelineLayout;
  sky.name = "sky";
  sky.data = {};
  // default sky parameters
  sky.data.data1 = glm::vec4(0.1, 0.2, 0.4, 0.97);

  VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1,
                                    &computePipelineCreateInfo, nullptr,
                                    &sky.pipeline));

  // add the 2 background effects into the array
  backgroundEffects.push_back(gradient);
  backgroundEffects.push_back(sky);

  // destroy structures properly
  vkDestroyShaderModule(_device, gradientShader, nullptr);
  vkDestroyShaderModule(_device, skyShader, nullptr);
  _mainDeletionQueue.push_function(
      [=]()
      {
        vkDestroyPipelineLayout(_device, _gradientPipelineLayout, nullptr);
        vkDestroyPipeline(_device, sky.pipeline, nullptr);
        vkDestroyPipeline(_device, gradient.pipeline, nullptr);
      });
}
void internal::init_triangle_pipeline()
{
  VkShaderModule triangleFragShader;
  if (!vkutil::load_shader_module(
          io::GetPath("shaders/colored_triangle.frag.vk.spv").c_str(), _device,
          &triangleFragShader))
  {
    log::Error("Error when building the triangle fragment shader module");
  }

  VkShaderModule triangleVertexShader;
  if (!vkutil::load_shader_module(
          io::GetPath("shaders/colored_triangle.vert.vk.spv").c_str(), _device,
          &triangleVertexShader))
  {
    log::Error("Error when building the triangle vertex shader module");
  }

  // build the pipeline layout that controls the inputs/outputs of the shader
  // we are not using descriptor sets or other systems yet, so no need to use
  // anything other than empty default
  VkPipelineLayoutCreateInfo pipeline_layout_info =
      vkinit::pipeline_layout_create_info();
  VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr,
                                  &_trianglePipelineLayout));
  vkutil::PipelineBuilder pipelineBuilder;

  // use the triangle layout we created
  pipelineBuilder._pipelineLayout = _trianglePipelineLayout;
  // connecting the vertex and pixel shaders to the pipeline
  pipelineBuilder.set_shaders(triangleVertexShader, triangleFragShader);
  // it will draw triangles
  pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  // filled triangles
  pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
  // no backface culling
  pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
  // no multisampling
  pipelineBuilder.set_multisampling_none();
  // no blending
  pipelineBuilder.disable_blending();

  pipelineBuilder.disable_depthtest();
  // pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

  // connect the image format we will draw into, from draw image
  pipelineBuilder.set_color_attachment_format(_drawImage.imageFormat);
  pipelineBuilder.set_depth_format(_depthImage.imageFormat);

  // finally build the pipeline
  _trianglePipeline = pipelineBuilder.build_pipeline(_device);

  // clean structures
  vkDestroyShaderModule(_device, triangleFragShader, nullptr);
  vkDestroyShaderModule(_device, triangleVertexShader, nullptr);

  _mainDeletionQueue.push_function(
      [&]()
      {
        vkDestroyPipelineLayout(_device, _trianglePipelineLayout, nullptr);
        vkDestroyPipeline(_device, _trianglePipeline, nullptr);
      });
}
void internal::init_descriptors()
{
  // create a descriptor pool that will hold 10 sets with 1 image each
  std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}};

  globalDescriptorAllocator.init(_device, 10, sizes);

  // make the descriptor set layout for our compute draw
  {
    DescriptorLayoutBuilder builder;
    builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    _drawImageDescriptorLayout =
        builder.build(_device, VK_SHADER_STAGE_COMPUTE_BIT);

    // allocate a descriptor set for our draw image
    _drawImageDescriptors =
        globalDescriptorAllocator.allocate(_device, _drawImageDescriptorLayout);

    DescriptorWriter writer;
    writer.write_image(0, _drawImage.imageView, VK_NULL_HANDLE,
                       VK_IMAGE_LAYOUT_GENERAL,
                       VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

    writer.update_set(_device, _drawImageDescriptors);

    // make sure both the descriptor allocator and the new layout get cleaned up
    // properly
    _mainDeletionQueue.push_function(
        [&]()
        {
          globalDescriptorAllocator.destroy_pools(_device);

          vkDestroyDescriptorSetLayout(_device, _drawImageDescriptorLayout,
                                       nullptr);
        });

    {
      DescriptorLayoutBuilder builder;
      builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
      _gpuSceneDataDescriptorLayout = builder.build(
          _device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    }
    {
      DescriptorLayoutBuilder builder;
      builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
      _singleImageDescriptorLayout =
          builder.build(_device, VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    for (int i = 0; i < FRAME_OVERLAP; i++)
    {
      // create a descriptor pool
      std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = {
          {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
          {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
          {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
          {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
      };

      _frames[i]._frameDescriptors = DescriptorAllocatorGrowable {};
      _frames[i]._frameDescriptors.init(_device, 1000, frame_sizes);

      _mainDeletionQueue.push_function(
          [&, i]()
          {
            _frames[i]._frameDescriptors.destroy_pools(_device);
          });
    }
  }
}
void internal::init_default_data()
{
  std::array<Vertex, 4> rect_vertices;

  rect_vertices[0].position = {0.5, -0.5, 0};
  rect_vertices[1].position = {0.5, 0.5, 0};
  rect_vertices[2].position = {-0.5, -0.5, 0};
  rect_vertices[3].position = {-0.5, 0.5, 0};

  rect_vertices[0].color = {0, 0, 0, 1};
  rect_vertices[1].color = {0.5, 0.5, 0.5, 1};
  rect_vertices[2].color = {1, 0, 0, 1};
  rect_vertices[3].color = {0, 1, 0, 1};

  std::array<uint32_t, 6> rect_indices;

  rect_indices[0] = 0;
  rect_indices[1] = 1;
  rect_indices[2] = 2;

  rect_indices[3] = 2;
  rect_indices[4] = 1;
  rect_indices[5] = 3;

  rectangle = UploadMesh(rect_indices, rect_vertices);

  // delete the rectangle data on engine shutdown
  _mainDeletionQueue.push_function(
      [&]()
      {
        destroy_buffer(rectangle.indexBuffer);
        destroy_buffer(rectangle.vertexBuffer);
      });

  // 3 default textures, white, grey, black. 1 pixel each
  uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
  _whiteImage =
      create_image((void*)&white, VkExtent3D {1, 1, 1},
                   VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

  uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
  _greyImage =
      create_image((void*)&grey, VkExtent3D {1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM,
                   VK_IMAGE_USAGE_SAMPLED_BIT);

  uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
  _blackImage =
      create_image((void*)&black, VkExtent3D {1, 1, 1},
                   VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

  // checkerboard image
  uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
  std::array<uint32_t, 16 * 16> pixels; // for 16x16 checkerboard texture
  for (int x = 0; x < 16; x++)
  {
    for (int y = 0; y < 16; y++)
    {
      pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
    }
  }
  _errorCheckerboardImage =
      create_image(pixels.data(), VkExtent3D {16, 16, 1},
                   VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

  VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

  sampl.magFilter = VK_FILTER_NEAREST;
  sampl.minFilter = VK_FILTER_NEAREST;

  vkCreateSampler(_device, &sampl, nullptr, &_defaultSamplerNearest);

  sampl.magFilter = VK_FILTER_LINEAR;
  sampl.minFilter = VK_FILTER_LINEAR;
  vkCreateSampler(_device, &sampl, nullptr, &_defaultSamplerLinear);

  _mainDeletionQueue.push_function(
      [&]()
      {
        vkDestroySampler(_device, _defaultSamplerNearest, nullptr);
        vkDestroySampler(_device, _defaultSamplerLinear, nullptr);

        destroy_image(_whiteImage);
        destroy_image(_greyImage);
        destroy_image(_blackImage);
        destroy_image(_errorCheckerboardImage);
      });

  GLTFMetallic_Roughness::MaterialResources materialResources;
  // default the material textures
  materialResources.colorImage = _whiteImage;
  materialResources.colorSampler = _defaultSamplerLinear;
  materialResources.metalRoughImage = _whiteImage;
  materialResources.metalRoughSampler = _defaultSamplerLinear;

  // set the uniform buffer for the material data
  AllocatedBuffer materialConstants = create_buffer(
      sizeof(GLTFMetallic_Roughness::MaterialConstants),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  // write the buffer
  GLTFMetallic_Roughness::MaterialConstants* sceneUniformData =
      static_cast<GLTFMetallic_Roughness::MaterialConstants*>(
          materialConstants.allocation->GetMappedData());
  sceneUniformData->colorFactors = glm::vec4 {1, 1, 1, 1};
  sceneUniformData->metal_rough_factors = glm::vec4 {1, 0.5, 0, 0};

  _mainDeletionQueue.push_function(
      [=]()
      {
        destroy_buffer(materialConstants);
      });

  materialResources.dataBuffer = materialConstants.buffer;
  materialResources.dataBufferOffset = 0;

  defaultData = metalRoughMaterial.write_material(
      _device, MaterialPass::MainColor, materialResources,
      globalDescriptorAllocator);
  // TODO move to a proper function
  testMeshes = loadGltfMeshes(io::GetPath("models/basicmesh.glb")).value();
  for (auto& m : testMeshes)
  {
    std::shared_ptr<MeshNode> newNode = std::make_shared<MeshNode>();
    newNode->mesh = m;

    newNode->localTransform = glm::mat4 {1.f};
    newNode->worldTransform = glm::mat4 {1.f};

    for (auto& s : newNode->mesh->surfaces)
    {
      s.material = std::make_shared<GLTFMaterial>(defaultData);
    }

    loadedNodes[m->name] = std::move(newNode);
  }

  std::string structurePath = {io::GetPath("models/structure.glb")};
  const auto structureFile = loadGltf(_device, structurePath);

  assert(structureFile.has_value());

  loadedScenes["structure"] = *structureFile;
}
void internal::init_mesh_pipeline()
{
  VkShaderModule triangleFragShader;
  if (!vkutil::load_shader_module(
          io::GetPath("shaders/tex_image.frag.vk.spv").c_str(), _device,
          &triangleFragShader))
  {
    log::Error("Error when building the triangle fragment shader module");
  }
  else
  {
    log::Info("Triangle fragment shader succesfully loaded");
  }

  VkShaderModule triangleVertexShader;
  if (!vkutil::load_shader_module(
          io::GetPath("shaders/colored_triangle_mesh.vert.vk.spv").c_str(),
          _device, &triangleVertexShader))
  {
    log::Error("Error when building the triangle vertex shader module");
  }
  else
  {
    log::Info("Triangle vertex shader succesfully loaded");
  }

  VkPushConstantRange bufferRange {};
  bufferRange.offset = 0;
  bufferRange.size = sizeof(GPUDrawPushConstants);
  bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkPipelineLayoutCreateInfo pipeline_layout_info =
      vkinit::pipeline_layout_create_info();
  pipeline_layout_info.pPushConstantRanges = &bufferRange;
  pipeline_layout_info.pushConstantRangeCount = 1;
  pipeline_layout_info.pSetLayouts = &_singleImageDescriptorLayout;
  pipeline_layout_info.setLayoutCount = 1;
  VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr,
                                  &_meshPipelineLayout));
  vkutil::PipelineBuilder pipelineBuilder;

  // use the triangle layout we created
  pipelineBuilder._pipelineLayout = _meshPipelineLayout;
  // connecting the vertex and pixel shaders to the pipeline
  pipelineBuilder.set_shaders(triangleVertexShader, triangleFragShader);
  // it will draw triangles
  pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  // filled triangles
  pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
  // no backface culling
  pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE,
                                VK_FRONT_FACE_COUNTER_CLOCKWISE);
  // no multisampling
  pipelineBuilder.set_multisampling_none();
  // no blending
  pipelineBuilder.disable_blending();
  // pipelineBuilder.enable_blending_additive();

  // pipelineBuilder.disable_depthtest();
  pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

  // connect the image format we will draw into, from draw image
  pipelineBuilder.set_color_attachment_format(_drawImage.imageFormat);
  pipelineBuilder.set_depth_format(_depthImage.imageFormat);

  // finally build the pipeline
  _meshPipeline = pipelineBuilder.build_pipeline(_device);

  // clean structures
  vkDestroyShaderModule(_device, triangleFragShader, nullptr);
  vkDestroyShaderModule(_device, triangleVertexShader, nullptr);

  _mainDeletionQueue.push_function(
      [&]()
      {
        vkDestroyPipelineLayout(_device, _meshPipelineLayout, nullptr);
        vkDestroyPipeline(_device, _meshPipeline, nullptr);
        vkDestroyDescriptorSetLayout(_device, _singleImageDescriptorLayout,
                                     nullptr);
      });
}
void hm::gpx::Renderer::Update(f32 dt) {}
void hm::gpx::Renderer::Render()
{
  external::ImGuiStartFrame();
  // some imgui UI to test
  ImGui::ShowDemoWindow();
  ImGui::Begin("Stats");

  ImGui::Text("frametime %f ms", stats.frametime);
  ImGui::Text("draw time %f ms", stats.mesh_draw_time);
  ImGui::Text("update time %f ms", stats.scene_update_time);
  ImGui::Text("triangles %i", stats.triangle_count);
  ImGui::Text("draws %i", stats.drawcall_count);
  ImGui::End();
  if (ImGui::Begin("background"))
  {
    ImGui::SliderFloat("Render Scale", &renderScale, 0.3f, 1.f);

    ComputeEffect& selected = backgroundEffects[currentBackgroundEffect];

    ImGui::Text("Selected effect: ", selected.name);

    ImGui::SliderInt("Effect Index", &currentBackgroundEffect, 0,
                     backgroundEffects.size() - 1);

    ImGui::InputFloat4("data1", (float*)&selected.data.data1);
    ImGui::InputFloat4("data2", (float*)&selected.data.data2);
    ImGui::InputFloat4("data3", (float*)&selected.data.data3);
    ImGui::InputFloat4("data4", (float*)&selected.data.data4);
  }
  ImGui::End();

  // wait until the gpu has finished rendering the last frame. Timeout of 1
  // second
  auto& mainCamera = Engine::Instance().GetECS().GetSystem<Camera>();

  update_scene(mainCamera);
  VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true,
                           1000000000));

  get_current_frame()._deletionQueue.flush();
  get_current_frame()._frameDescriptors.clear_pools(_device);

  VkResult e = vkAcquireNextImageKHR(_device, _swapchain, 1000000000,
                                     get_current_frame()._swapchainSemaphore,
                                     nullptr, &swapchainImageIndex);
  if (e == VK_ERROR_OUT_OF_DATE_KHR)
  {
    Engine::Instance().GetDevice().SetResizeRequest(true);
    return;
  }
  _drawExtent.height =
      std::min(_swapchainExtent.height, _drawImage.imageExtent.height) *
      renderScale;
  _drawExtent.width =
      std::min(_swapchainExtent.width, _drawImage.imageExtent.width) *
      renderScale;

  VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

  // naming it cmd for shorter writing
  VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;

  // now that we are sure that the commands finished executing, we can safely
  // reset the command buffer to begin recording again.
  VK_CHECK(vkResetCommandBuffer(get_current_frame()._mainCommandBuffer, 0));

  // begin the command buffer recording. We will use this command buffer exactly
  // once, so we want to let vulkan know that
  VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

  // transition our main draw image into general layout so we can write into it
  // we will overwrite it all so we dont care about what was the older layout
  vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_GENERAL);

  draw_background(cmd);

  vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL,
                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  vkutil::transition_image(cmd, _depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
  draw_geometry(cmd);

  // transtion the draw image and the swapchain image into their correct
  // transfer layouts
  vkutil::transition_image(cmd, _drawImage.image,
                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex],
                           VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  // execute a copy from the draw image into the swapchain
  vkutil::copy_image_to_image(cmd, _drawImage.image,
                              _swapchainImages[swapchainImageIndex],
                              _drawExtent, _swapchainExtent);

  // set swapchain image layout to Attachment Optimal so we can draw it
  vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex],
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  // draw imgui into the swapchain image
  external::ImGuiEndFrame();
  // set swapchain image layout to Present so we can draw it
  vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex],
                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                           VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  // finalize the command buffer (we can no longer add commands, but it can now
  // be executed)
  VK_CHECK(vkEndCommandBuffer(cmd));

  // prepare the submission to the queue.
  // we want to wait on the _presentSemaphore, as that semaphore is signaled
  // when the swapchain is ready we will signal the _renderSemaphore, to signal
  // that rendering has finished

  VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);

  VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(
      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
      get_current_frame()._swapchainSemaphore);
  VkSemaphoreSubmitInfo signalInfo =
      vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                                    get_current_frame()._renderSemaphore);

  VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, &signalInfo, &waitInfo);

  // submit command buffer to the queue and execute it.
  //  _renderFence will now block until the graphic commands finish execution
  VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit,
                          get_current_frame()._renderFence));

  // prepare present
  // this will put the image we just rendered to into the visible window.
  // we want to wait on the _renderSemaphore for that,
  // as its necessary that drawing commands have finished before the image is
  // displayed to the user
  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.pNext = nullptr;
  presentInfo.pSwapchains = &_swapchain;
  presentInfo.swapchainCount = 1;

  presentInfo.pWaitSemaphores = &get_current_frame()._renderSemaphore;
  presentInfo.waitSemaphoreCount = 1;

  presentInfo.pImageIndices = &swapchainImageIndex;

  const VkResult presentResult =
      vkQueuePresentKHR(_graphicsQueue, &presentInfo);
  if (presentResult == VK_ERROR_OUT_OF_DATE_KHR)
  {
    Engine::Instance().GetDevice().SetResizeRequest(true);
    return;
  }

  // increase the number of frames drawn
  _frameNumber++;
}

void internal::draw_geometry(VkCommandBuffer cmd)
{
  // reset counters
  stats.drawcall_count = 0;
  stats.triangle_count = 0;
  // begin clock
  auto start = std::chrono::system_clock::now();

  // allocate a new uniform buffer for the scene data
  AllocatedBuffer gpuSceneDataBuffer =
      create_buffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VMA_MEMORY_USAGE_CPU_TO_GPU);

  // add it to the deletion queue of this frame so it gets deleted once its
  // been used
  get_current_frame()._deletionQueue.push_function(
      // TODO removed this
      [=]()
      {
        destroy_buffer(gpuSceneDataBuffer);
      });

  // write the buffer
  GPUSceneData* sceneUniformData = static_cast<GPUSceneData*>(
      gpuSceneDataBuffer.allocation->GetMappedData());
  *sceneUniformData = sceneData;

  // create a descriptor set that binds that buffer and update it
  VkDescriptorSet globalDescriptor =
      get_current_frame()._frameDescriptors.allocate(
          _device, _gpuSceneDataDescriptorLayout);

  DescriptorWriter writer;
  writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0,
                      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
  writer.update_set(_device, globalDescriptor);

  // begin a render pass  connected to our draw image

  VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(
      _drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
  VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(
      _depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

  VkRenderingInfo renderInfo =
      vkinit::rendering_info(_drawExtent, &colorAttachment, &depthAttachment);

  vkCmdBeginRendering(cmd, &renderInfo);

  std::vector<uint32_t> opaque_draws;
  opaque_draws.reserve(mainDrawContext.OpaqueSurfaces.size());

  for (int i = 0; i < mainDrawContext.OpaqueSurfaces.size(); i++)
  {
    if (is_visible(mainDrawContext.OpaqueSurfaces[i], sceneData.viewproj))
    {
      opaque_draws.push_back(i);
    }
  }

  // sort the opaque surfaces by material and mesh
  std::sort(opaque_draws.begin(), opaque_draws.end(),
            [&](const auto& iA, const auto& iB)
            {
              const RenderObject& A = mainDrawContext.OpaqueSurfaces[iA];
              const RenderObject& B = mainDrawContext.OpaqueSurfaces[iB];
              if (A.material == B.material)
              {
                return A.indexBuffer < B.indexBuffer;
              }
              else
              {
                return A.material < B.material;
              }
            });

  // defined outside of the draw function, this is the state we will try to skip
  MaterialPipeline* lastPipeline = nullptr;
  MaterialInstance* lastMaterial = nullptr;
  VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

  auto draw = [&](const RenderObject& r)
  {
    if (r.material != lastMaterial)
    {
      lastMaterial = r.material;
      // rebind pipeline and descriptors if the material changed
      if (r.material->pipeline != lastPipeline)
      {
        lastPipeline = r.material->pipeline;
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          r.material->pipeline->pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                r.material->pipeline->layout, 0, 1,
                                &globalDescriptor, 0, nullptr);

        VkViewport viewport = {};
        auto windowSize = Engine::Instance().GetDevice().GetWindowSize();
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = static_cast<float>(windowSize.x);
        viewport.height = static_cast<float>(windowSize.y);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent.width = windowSize.x;
        scissor.extent.height = windowSize.y;

        vkCmdSetScissor(cmd, 0, 1, &scissor);
      }

      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              r.material->pipeline->layout, 1, 1,
                              &r.material->materialSet, 0, nullptr);
    }
    // rebind index buffer if needed
    if (r.indexBuffer != lastIndexBuffer)
    {
      lastIndexBuffer = r.indexBuffer;
      vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    }
    // calculate final mesh matrix
    GPUDrawPushConstants push_constants;
    push_constants.worldMatrix = r.transform;
    push_constants.vertexBuffer = r.vertexBufferAddress;

    vkCmdPushConstants(cmd, r.material->pipeline->layout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(GPUDrawPushConstants), &push_constants);

    vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
    // stats
    stats.drawcall_count++;
    stats.triangle_count += r.indexCount / 3;
  };

  for (auto& r : opaque_draws)
  {
    draw(mainDrawContext.OpaqueSurfaces[r]);
  }

  for (auto& r : mainDrawContext.TransparentSurfaces)
  {
    draw(r);
  }

  vkCmdEndRendering(cmd);

  auto end = std::chrono::system_clock::now();

  // convert to microseconds (integer), and then come back to miliseconds
  auto elapsed =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  stats.mesh_draw_time = elapsed.count() / 1000.f;
}
void GLTFMetallic_Roughness::clear_resources(VkDevice device) {}
void GLTFMetallic_Roughness::build_pipelines()
{
  VkShaderModule meshFragShader;
  if (!vkutil::load_shader_module(
          io::GetPath("shaders/mesh.frag.vk.spv").c_str(), _device,
          &meshFragShader))
  {
    log::Error("Error when building the fragment shader module");
  }
  VkShaderModule meshVertexShader;
  if (!vkutil::load_shader_module(
          io::GetPath("shaders/mesh.vert.vk.spv").c_str(), _device,
          &meshVertexShader))
  {
    hm::log::Error("Failed building the vertex shader module");
  }

  VkPushConstantRange matrixRange {};
  matrixRange.offset = 0;
  matrixRange.size = sizeof(GPUDrawPushConstants);
  matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  DescriptorLayoutBuilder layoutBuilder;
  layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
  layoutBuilder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
  layoutBuilder.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

  materialLayout = layoutBuilder.build(
      _device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

  VkDescriptorSetLayout layouts[] = {_gpuSceneDataDescriptorLayout,
                                     materialLayout};

  VkPipelineLayoutCreateInfo mesh_layout_info =
      vkinit::pipeline_layout_create_info();
  mesh_layout_info.setLayoutCount = 2;
  mesh_layout_info.pSetLayouts = layouts;
  mesh_layout_info.pPushConstantRanges = &matrixRange;
  mesh_layout_info.pushConstantRangeCount = 1;

  VkPipelineLayout newLayout;
  VK_CHECK(
      vkCreatePipelineLayout(_device, &mesh_layout_info, nullptr, &newLayout));

  opaquePipeline.layout = newLayout;
  transparentPipeline.layout = newLayout;

  // build the stage-create-info for both vertex and fragment stages. This
  // lets the pipeline know the shader modules per stage
  vkutil::PipelineBuilder pipelineBuilder;
  pipelineBuilder.set_shaders(meshVertexShader, meshFragShader);
  pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
  pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
  pipelineBuilder.set_multisampling_none();
  pipelineBuilder.disable_blending();
  pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

  // render format
  pipelineBuilder.set_color_attachment_format(_drawImage.imageFormat);
  pipelineBuilder.set_depth_format(_depthImage.imageFormat);

  // use the triangle layout we created
  pipelineBuilder._pipelineLayout = newLayout;

  // finally build the pipeline
  opaquePipeline.pipeline = pipelineBuilder.build_pipeline(_device);

  // create the transparent variant
  pipelineBuilder.enable_blending_additive();

  pipelineBuilder.enable_depthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

  transparentPipeline.pipeline = pipelineBuilder.build_pipeline(_device);

  vkDestroyShaderModule(_device, meshFragShader, nullptr);
  vkDestroyShaderModule(_device, meshVertexShader, nullptr);
  _mainDeletionQueue.push_function(
      [=]()
      {
        vkDestroyPipelineLayout(_device, opaquePipeline.layout, nullptr);

        vkDestroyPipeline(_device, opaquePipeline.pipeline, nullptr);

        vkDestroyPipeline(_device, transparentPipeline.pipeline, nullptr);
        vkDestroyDescriptorSetLayout(_device, materialLayout, nullptr);
        vkDestroyDescriptorSetLayout(_device, _gpuSceneDataDescriptorLayout,
                                     nullptr);
      });
}
GPUMeshBuffers hm::UploadMesh(std::span<uint32_t> indices,
                              std::span<Vertex> vertices)
{
  const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
  const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

  GPUMeshBuffers newSurface;

  // create vertex buffer
  newSurface.vertexBuffer = create_buffer(
      vertexBufferSize,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY);

  // find the adress of the vertex buffer
  VkBufferDeviceAddressInfo deviceAdressInfo {
      .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
      .buffer = newSurface.vertexBuffer.buffer};
  newSurface.vertexBufferAddress =
      vkGetBufferDeviceAddress(_device, &deviceAdressInfo);

  // create index buffer
  newSurface.indexBuffer = create_buffer(
      indexBufferSize,
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY);

  AllocatedBuffer staging = create_buffer(vertexBufferSize + indexBufferSize,
                                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                          VMA_MEMORY_USAGE_CPU_ONLY);

  void* data = staging.allocation->GetMappedData();

  // copy vertex buffer
  memcpy(data, vertices.data(), vertexBufferSize);
  // copy index buffer
  memcpy(static_cast<char*>(data) + vertexBufferSize, indices.data(),
         indexBufferSize);

  immediate_submit(
      [&](VkCommandBuffer cmd)
      {
        VkBufferCopy vertexCopy {0};
        vertexCopy.dstOffset = 0;
        vertexCopy.srcOffset = 0;
        vertexCopy.size = vertexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, newSurface.vertexBuffer.buffer, 1,
                        &vertexCopy);

        VkBufferCopy indexCopy {0};
        indexCopy.dstOffset = 0;
        indexCopy.srcOffset = vertexBufferSize;
        indexCopy.size = indexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, newSurface.indexBuffer.buffer, 1,
                        &indexCopy);
      });

  destroy_buffer(staging);

  return newSurface;
}
void internal::draw_background(VkCommandBuffer cmd)

{
  ComputeEffect& effect = backgroundEffects[currentBackgroundEffect];

  // bind the background compute pipeline
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);

  // bind the descriptor set containing the draw image for the compute pipeline
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                          _gradientPipelineLayout, 0, 1, &_drawImageDescriptors,
                          0, nullptr);

  vkCmdPushConstants(cmd, _gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
                     0, sizeof(ComputePushConstants), &effect.data);
  // execute the compute pipeline dispatch. We are using 16x16 workgroup size so
  // we need to divide by it
  vkCmdDispatch(cmd, std::ceil(_drawExtent.width / 16.0),
                std::ceil(_drawExtent.height / 16.0), 1);
}
void internal::update_scene(Camera& mainCamera)
{
  auto start = std::chrono::system_clock::now();
  mainDrawContext.OpaqueSurfaces.clear();
  mainDrawContext.TransparentSurfaces.clear();
  loadedScenes["structure"]->Draw(glm::mat4 {1.f}, mainDrawContext);

  loadedNodes["Suzanne"]->Draw(glm::mat4 {1.f}, mainDrawContext);

  glm::mat4 view = mainCamera.getViewMatrix();

  // camera projection
  auto windowExtent = Engine::Instance().GetDevice().GetWindowSize();
  glm::mat4 projection = glm::perspective(
      glm::radians(70.f),
      static_cast<float>(windowExtent.x) / static_cast<float>(windowExtent.y),
      10000.f, 0.1f);

  // invert the Y direction on projection matrix so that we are more similar
  // to opengl and gltf axis
  projection[1][1] *= -1;

  sceneData.view = view;
  sceneData.proj = projection;
  sceneData.viewproj = projection * view;

  // some default lighting parameters
  sceneData.ambientColor = glm::vec4(.1f);
  sceneData.sunlightColor = glm::vec4(1.f);
  sceneData.sunlightDirection = glm::vec4(0, 1, 0.5, 1.f);

  for (int x = -3; x < 3; x++)
  {
    glm::mat4 scale = glm::scale(glm::identity<glm::mat4>(), glm::vec3 {0.2});
    glm::mat4 translation =
        glm::translate(glm::identity<glm::mat4>(), glm::vec3 {x, 1, 0});

    loadedNodes["Cube"]->Draw(translation * scale, mainDrawContext);
  }
  auto end = std::chrono::system_clock::now();

  // convert to microseconds (integer), and then come back to miliseconds
  auto elapsed =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  stats.scene_update_time = elapsed.count() / 1000.f;
}
