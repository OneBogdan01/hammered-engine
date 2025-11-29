#include "platform/vulkan/pipelines_vk.hpp"

#include "glslang_c_interface.h"
#include "volk.h"
#include "glslang/Public/resource_limits_c.h"
#include "utility/logger.hpp"

using namespace vkutil;
void PrintShaderSource(const char* text)
{
  int line = 1;

  printf("\n(%3i) ", line);

  while (text && *text++)
  {
    if (*text == '\n')
    {
      printf("\n(%3i) ", ++line);
    }
    else if (*text == '\r')
    {
    }
    else
    {
      printf("%c", *text);
    }
  }

  printf("\n");
}
bool vkutil::load_shader_module(const char* filePath, VkDevice device,
                                VkShaderModule* outShaderModule)
{
  // open the file. With cursor at the end
  std::ifstream file(filePath, std::ios::ate | std::ios::binary);

  if (!file.is_open())
  {
    return false;
  }

  // find what the size of the file is by looking up the location of the cursor
  // because the cursor is at the end, it gives the size directly in bytes
  size_t fileSize = (size_t)file.tellg();

  // spirv expects the buffer to be on uint32, so make sure to reserve a int
  // vector big enough for the entire file
  std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

  // put file cursor at beginning
  file.seekg(0);

  // load the entire file into the buffer
  file.read((char*)buffer.data(), fileSize);

  // now that the file is loaded into the buffer, we can close it
  file.close();

  // create a new shader module, using the buffer we loaded
  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.pNext = nullptr;

  // codeSize has to be in bytes, so multply the ints in the buffer by size of
  // int to know the real size of the buffer
  createInfo.codeSize = buffer.size() * sizeof(uint32_t);
  createInfo.pCode = buffer.data();

  // check that the creation goes well.
  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
      VK_SUCCESS)
  {
    return false;
  }
  *outShaderModule = shaderModule;
  return true;
}
void PipelineBuilder::clear()
{
  // clear all of the structs we need back to 0 with their correct stype

  _inputAssembly = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};

  _rasterizer = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};

  _colorBlendAttachment = {};

  _multisampling = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};

  _pipelineLayout = {};

  _depthStencil = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};

  _renderInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};

  _shaderStages.clear();
}
VkPipeline PipelineBuilder::build_pipeline(VkDevice device)
{
  // make viewport state from our stored viewport and scissor.
  // at the moment we wont support multiple viewports or scissors
  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.pNext = nullptr;

  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  // setup dummy color blending. We arent using transparent objects yet
  // the blending is just "no blend", but we do write to the color attachment
  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.pNext = nullptr;

  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &_colorBlendAttachment;

  // completely clear VertexInputStateCreateInfo, as we have no need for it
  VkPipelineVertexInputStateCreateInfo _vertexInputInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  // build the actual pipeline
  // we now use all of the info structs we have been writing into into this one
  // to create the pipeline
  VkGraphicsPipelineCreateInfo pipelineInfo = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
  // connect the renderInfo to the pNext extension mechanism
  pipelineInfo.pNext = &_renderInfo;

  pipelineInfo.stageCount = (uint32_t)_shaderStages.size();
  pipelineInfo.pStages = _shaderStages.data();
  pipelineInfo.pVertexInputState = &_vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &_inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &_rasterizer;
  pipelineInfo.pMultisampleState = &_multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDepthStencilState = &_depthStencil;
  pipelineInfo.layout = _pipelineLayout;

  VkDynamicState state[] = {VK_DYNAMIC_STATE_VIEWPORT,
                            VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamicInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
  dynamicInfo.pDynamicStates = &state[0];
  dynamicInfo.dynamicStateCount = 2;

  pipelineInfo.pDynamicState = &dynamicInfo;

  // its easy to error out on create graphics pipeline, so we handle it a bit
  // better than the common VK_CHECK case
  VkPipeline newPipeline;
  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &newPipeline) != VK_SUCCESS)
  {
    hm::log::Warning("failed to create pipeline");
    return VK_NULL_HANDLE; // failed to create graphics pipeline
  }
  else
  {
    return newPipeline;
  }
}
void PipelineBuilder::set_shaders(VkShaderModule vertexShader,
                                  VkShaderModule fragmentShader)
{
  _shaderStages.clear();

  _shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(
      VK_SHADER_STAGE_VERTEX_BIT, vertexShader));

  _shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(
      VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
}
void PipelineBuilder::set_input_topology(VkPrimitiveTopology topology)
{
  _inputAssembly.topology = topology;
  // we are not going to use primitive restart on the entire tutorial so leave
  // it on false
  _inputAssembly.primitiveRestartEnable = VK_FALSE;
}

void PipelineBuilder::set_polygon_mode(VkPolygonMode mode)
{
  _rasterizer.polygonMode = mode;
  _rasterizer.lineWidth = 1.f;
}

void PipelineBuilder::set_cull_mode(VkCullModeFlags cullMode,
                                    VkFrontFace frontFace)
{
  _rasterizer.cullMode = cullMode;
  _rasterizer.frontFace = frontFace;
}

void PipelineBuilder::set_multisampling_none()
{
  _multisampling.sampleShadingEnable = VK_FALSE;
  // multisampling defaulted to no multisampling (1 sample per pixel)
  _multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  _multisampling.minSampleShading = 1.0f;
  _multisampling.pSampleMask = nullptr;
  // no alpha to coverage either
  _multisampling.alphaToCoverageEnable = VK_FALSE;
  _multisampling.alphaToOneEnable = VK_FALSE;
}
void PipelineBuilder::disable_blending()
{
  // default write mask
  _colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  // no blending
  _colorBlendAttachment.blendEnable = VK_FALSE;
}
void PipelineBuilder::set_color_attachment_format(VkFormat format)
{
  _colorAttachmentformat = format;
  // connect the format to the renderInfo  structure
  _renderInfo.colorAttachmentCount = 1;
  _renderInfo.pColorAttachmentFormats = &_colorAttachmentformat;
}

void PipelineBuilder::set_depth_format(VkFormat format)
{
  _renderInfo.depthAttachmentFormat = format;
}
void PipelineBuilder::disable_depthtest()
{
  _depthStencil.depthTestEnable = VK_FALSE;
  _depthStencil.depthWriteEnable = VK_FALSE;
  _depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
  _depthStencil.depthBoundsTestEnable = VK_FALSE;
  _depthStencil.stencilTestEnable = VK_FALSE;
  _depthStencil.front = {};
  _depthStencil.back = {};
  _depthStencil.minDepthBounds = 0.f;
  _depthStencil.maxDepthBounds = 1.f;
}
void PipelineBuilder::enable_depthtest(bool depthWriteEnable, VkCompareOp op)
{
  _depthStencil.depthTestEnable = VK_TRUE;
  _depthStencil.depthWriteEnable = depthWriteEnable;
  _depthStencil.depthCompareOp = op;
  _depthStencil.depthBoundsTestEnable = VK_FALSE;
  _depthStencil.stencilTestEnable = VK_FALSE;
  _depthStencil.front = {};
  _depthStencil.back = {};
  _depthStencil.minDepthBounds = 0.f;
  _depthStencil.maxDepthBounds = 1.f;
}

void PipelineBuilder::enable_blending_additive()
{
  _colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  _colorBlendAttachment.blendEnable = VK_TRUE;
  _colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  _colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
  _colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  _colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  _colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  _colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

void PipelineBuilder::enable_blending_alphablend()
{
  _colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  _colorBlendAttachment.blendEnable = VK_TRUE;
  _colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  _colorBlendAttachment.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  _colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  _colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  _colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  _colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

size_t hm::vk::CompileShader(glslang_stage_t stage, const char* shaderSource,
                             ShaderModule& shaderModule)
{
  std::string modifiedSource = std::string(shaderSource);
  const glslang_input_t input = {
      .language = GLSLANG_SOURCE_GLSL,
      .stage = stage,
      .client = GLSLANG_CLIENT_VULKAN,
      .client_version = GLSLANG_TARGET_VULKAN_1_1,
      .target_language = GLSLANG_TARGET_SPV,
      .target_language_version = GLSLANG_TARGET_SPV_1_3,
      .code = modifiedSource.c_str(),
      .default_version = 450,
      .default_profile = GLSLANG_CORE_PROFILE,
      .force_default_version_and_profile = false,
      .forward_compatible = false,
      .messages = GLSLANG_MSG_DEFAULT_BIT,
      .resource = glslang_default_resource(),
  };

  glslang_shader_t* shader = glslang_shader_create(&input);

  if (!glslang_shader_preprocess(shader, &input))
  {
    fprintf(stderr, "GLSL preprocessing failed\n");
    fprintf(stderr, "\n%s", glslang_shader_get_info_log(shader));
    fprintf(stderr, "\n%s", glslang_shader_get_info_debug_log(shader));
    PrintShaderSource(input.code);
    return 0;
  }

  if (!glslang_shader_parse(shader, &input))
  {
    fprintf(stderr, "GLSL parsing failed\n");
    fprintf(stderr, "\n%s", glslang_shader_get_info_log(shader));
    fprintf(stderr, "\n%s", glslang_shader_get_info_debug_log(shader));
    PrintShaderSource(glslang_shader_get_preprocessed_code(shader));
    return 0;
  }

  glslang_program_t* program = glslang_program_create();
  glslang_program_add_shader(program, shader);

  if (!glslang_program_link(
          program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT))
  {
    fprintf(stderr, "GLSL linking failed\n");
    fprintf(stderr, "\n%s", glslang_program_get_info_log(program));
    fprintf(stderr, "\n%s", glslang_program_get_info_debug_log(program));
    return 0;
  }

  glslang_program_SPIRV_generate(program, stage);

  shaderModule.SPIRV.resize(glslang_program_SPIRV_get_size(program));
  glslang_program_SPIRV_get(program, shaderModule.SPIRV.data());

  {
    const char* spirv_messages = glslang_program_SPIRV_get_messages(program);

    if (spirv_messages)
      fprintf(stderr, "%s", spirv_messages);
  }

  glslang_program_delete(program);
  glslang_shader_delete(shader);

  return shaderModule.SPIRV.size();
}
std::string hm::vk::ReadShaderFile(const char* fileName)
{
  FILE* file = fopen(fileName, "r");

  if (!file)
  {
    printf("I/O error. Cannot open shader file '%s'\n", fileName);
    return std::string();
  }

  fseek(file, 0L, SEEK_END);
  const auto bytesinfile = ftell(file);
  fseek(file, 0L, SEEK_SET);

  char* buffer = (char*)alloca(bytesinfile + 1);
  const size_t bytesread = fread(buffer, 1, bytesinfile, file);
  fclose(file);

  buffer[bytesread] = 0;

  static constexpr unsigned char BOM[] = {0xEF, 0xBB, 0xBF};

  if (bytesread > 3)
  {
    if (!memcmp(buffer, BOM, 3))
      memset(buffer, ' ', 3);
  }

  std::string code(buffer);

  while (code.find("#include ") != code.npos)
  {
    const auto pos = code.find("#include ");
    const auto p1 = code.find('<', pos);
    const auto p2 = code.find('>', pos);
    if (p1 == code.npos || p2 == code.npos || p2 <= p1)
    {
      printf("Error while loading shader program: %s\n", code.c_str());
      return std::string();
    }
    const std::string name = code.substr(p1 + 1, p2 - p1 - 1);
    const std::string include = ReadShaderFile(name.c_str());
    code.replace(pos, p2 - pos + 1, include.c_str());
  }

  return code;
}
bool endsWith(const char* s, const char* part)
{
  const size_t sLength = strlen(s);
  const size_t partLength = strlen(part);
  if (sLength < partLength)
    return false;
  return strcmp(s + sLength - partLength, part) == 0;
}
glslang_stage_t glslangShaderStageFromFileName(const char* fileName)
{
  if (endsWith(fileName, ".vert"))
    return GLSLANG_STAGE_VERTEX;

  if (endsWith(fileName, ".frag"))
    return GLSLANG_STAGE_FRAGMENT;

  if (endsWith(fileName, ".geom"))
    return GLSLANG_STAGE_GEOMETRY;

  if (endsWith(fileName, ".comp"))
    return GLSLANG_STAGE_COMPUTE;

  if (endsWith(fileName, ".tesc"))
    return GLSLANG_STAGE_TESSCONTROL;

  if (endsWith(fileName, ".tese"))
    return GLSLANG_STAGE_TESSEVALUATION;

  return GLSLANG_STAGE_VERTEX;
}
size_t hm::vk::CompileShaderFile(const char* file,
                                 hm::vk::ShaderModule& shaderModule)
{
  if (auto shaderSource = ReadShaderFile(file); !shaderSource.empty())
    return CompileShader(glslangShaderStageFromFileName(file),
                         shaderSource.c_str(), shaderModule);

  return 0;
}
void hm::vk::CompilerShaderModule(const char* sourceFilename,
                                  const char* destFilename)
{
  ShaderModule shaderModule;

  if (CompileShaderFile(sourceFilename, shaderModule) < 1)
    return;

  SaveSPIRVBinaryFile(destFilename, shaderModule.SPIRV.data(),
                      shaderModule.SPIRV.size());
}
void hm::vk::SaveSPIRVBinaryFile(const char* filename, unsigned* code,
                                 size_t size)
{
  FILE* f = fopen(filename, "wb");

  if (!f)
    return;

  fwrite(code, sizeof(uint32_t), size, f);
  fclose(f);
}
