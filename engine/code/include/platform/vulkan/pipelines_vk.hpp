#pragma once
#include "platform/vulkan/types_vk.hpp"
#include "platform/vulkan/pipelines_vk.hpp"
#include "platform/vulkan/initializers_vk.hpp"
#include <glslang_c_shader_types.h>

namespace hm::vk
{

struct ShaderModule
{
  std::vector<u32> SPIRV {};
  VkShaderModule shaderModule {};
};
size_t CompileShader(glslang_stage_t stage, const char* shaderSource,
                     ShaderModule& shaderModule);
std::string ReadShaderFile(const char* fileName);
size_t CompileShaderFile(const char* file, hm::vk::ShaderModule& shaderModule);
void CompilerShaderModule(const char* sourceFilename, const char* destFilename);
void SaveSPIRVBinaryFile(const char* filename, unsigned int* code, size_t size);
} // namespace hm::vk
namespace vkutil
{
bool load_shader_module(const char* filePath, VkDevice device,
                        VkShaderModule* outShaderModule);
class PipelineBuilder
{
 public:
  std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;

  VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
  VkPipelineRasterizationStateCreateInfo _rasterizer;
  VkPipelineColorBlendAttachmentState _colorBlendAttachment;
  VkPipelineMultisampleStateCreateInfo _multisampling;
  VkPipelineLayout _pipelineLayout;
  VkPipelineDepthStencilStateCreateInfo _depthStencil;
  VkPipelineRenderingCreateInfo _renderInfo;
  VkFormat _colorAttachmentformat;

  PipelineBuilder() { clear(); }

  void clear();

  VkPipeline build_pipeline(VkDevice device);
  void set_shaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
  void set_input_topology(VkPrimitiveTopology topology);
  void set_polygon_mode(VkPolygonMode mode);
  void set_cull_mode(VkCullModeFlags cullMode, VkFrontFace frontFace);
  void set_multisampling_none();
  void disable_blending();
  void set_color_attachment_format(VkFormat format);
  void set_depth_format(VkFormat format);
  void disable_depthtest();
  void enable_depthtest(bool depthWriteEnable, VkCompareOp op);
  void enable_blending_additive();
  void enable_blending_alphablend();
};
}; // namespace vkutil
