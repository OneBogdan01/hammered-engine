#include "platform/opengl/shader_gl.hpp"

#include "utility/console.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <SDL3/SDL_assert.h>
std::string hm::ShaderPaths::GetPath() const
{
  std::string path {};
  if (Vertex.empty() == false)
  {
    path += Vertex + " ";
  }
  if (Fragment.empty() == false)
  {
    path += Fragment + " ";
  }
  if (Compute.empty() == false)
  {
    path += Compute + " ";
  }
  return path;
}
hm::Shader::Shader(const ShaderPaths& shader) : m_shaderPaths(shader)
{
  Reload();
  m_resourcePath = m_shaderPaths.GetPath();
}
hm::Shader::~Shader()
{
  glDeleteProgram(m_programId);
}

std::string hm::Shader::LoadShader(const std::string& path)
{
  std::ifstream shaderFile {path};

  return {std::istreambuf_iterator(shaderFile),
          std::istreambuf_iterator<char>()};
}

bool hm::Shader::CheckShaderCompilation(const GLuint& shaderId)
{
  GLint compiled {};
  glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compiled);
  if (compiled == GL_FALSE)
  {
#ifdef DEBUG

    GLint logLength {};
    glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);

    auto* log = new GLchar[logLength + 1];
    glGetShaderInfoLog(shaderId, logLength, &logLength, log);
    log::Error("Shader compilation failed: {}", log);
    delete[] log;

#endif
    glDeleteShader(shaderId);
    return false;
  }
  return true;
}
bool hm::Shader::CreateShader(GLuint& shaderId, GLenum shaderType,
                              const GLchar* source)
{
  if (source == nullptr)
  {
    log::Error("Empty shader");
    return false;
  }
  shaderId = glCreateShader(shaderType);
  return true;
}
bool hm::Shader::CompileShader(GLuint& shaderId, GLenum shaderType,
                               const GLchar* source)
{
  if (CreateShader(shaderId, shaderType, source) == false)
  {
    return false;
  }

  glShaderSource(shaderId, 1, &source, nullptr);

  glCompileShader(shaderId);

  return CheckShaderCompilation(shaderId);
}
bool hm::Shader::LinkProgram()
{
  glLinkProgram(m_programId);
  GLint linked;
  glGetProgramiv(m_programId, GL_LINK_STATUS, &linked);
  if (linked == GL_FALSE)
  {
#ifdef DEBUG
    GLsizei len;
    glGetProgramiv(m_programId, GL_INFO_LOG_LENGTH, &len);

    GLchar* log = new GLchar[len + 1];
    glGetProgramInfoLog(m_programId, len, &len, log);
    log::Error("Shader linking failed: {}", log);
    delete[] log;
#endif

    return false;
  }

  return true;
}

void DeleteShader(GLuint& id)
{
  if (id)
  {
    glDeleteShader(id);
    id = 0;
  }
}
std::vector<std::uint32_t> LoadShaderBinary(const std::string& filePath)
{
  // open the file. With cursor at the end
  std::ifstream file(filePath, std::ios::ate | std::ios::binary);

  if (!file.is_open())
  {
    hm::log::Error("Failed to find shader {}", filePath);

    return {};
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
  return buffer;
}
bool hm::Shader::CompileGLSL(const hm::ShaderPaths& sourcePath, ShaderIds& ids)
{
  // compile shaders
  if (m_shaderPaths.Vertex.empty() == false)
  {
    const auto success {CompileShader(ids.vertexId, GL_VERTEX_SHADER,
                                      sourcePath.Vertex.c_str())};

    if (success == false)
    {
      log::Error("Failed to compile vertex shader {}",
                 m_shaderPaths.Vertex.c_str());
      return false;
    }
  }
  if (m_shaderPaths.Fragment.empty() == false)
  {
    const auto success {CompileShader(ids.fragmentId, GL_FRAGMENT_SHADER,
                                      sourcePath.Fragment.c_str())};

    if (success == false)
    {
      log::Error("Failed to compile fragment shader {}",
                 m_shaderPaths.Fragment.c_str());
      return false;
    }
  }
  if (m_shaderPaths.Compute.empty() == false)
  {
    const auto success {CompileShader(ids.computeId, GL_COMPUTE_SHADER,
                                      sourcePath.Compute.c_str())};

    if (success == false)
    {
      log::Error("Failed to compile Compute shader {}",
                 m_shaderPaths.Compute.c_str());
      return false;
    }
  }
  return true;
}
void hm::Shader::DeleteShaders(ShaderIds& ids)
{
  DeleteShader(ids.vertexId);
  DeleteShader(ids.fragmentId);
  DeleteShader(ids.computeId);
}
bool hm::Shader::LoadSPIRV(const hm::ShaderPaths& sourcePath,
                           hm::ShaderIds& ids)
{
  if (m_shaderPaths.Vertex.empty() == false)
  {
    CreateShader(ids.vertexId, GL_VERTEX_SHADER, sourcePath.Vertex.c_str());

    const auto binary = LoadShaderBinary(sourcePath.Vertex);
    glShaderBinary(1, &ids.vertexId, GL_SHADER_BINARY_FORMAT_SPIR_V,
                   binary.data(), binary.size() * sizeof(uint32_t));

    glSpecializeShader(ids.vertexId, "main", 0, nullptr, nullptr);
    if (CheckShaderCompilation(ids.vertexId) == false)
    {
      return false;
    }
  }
  if (m_shaderPaths.Fragment.empty() == false)
  {
    CreateShader(ids.fragmentId, GL_FRAGMENT_SHADER,
                 sourcePath.Fragment.c_str());

    const auto binary = LoadShaderBinary(sourcePath.Fragment);
    glShaderBinary(1, &ids.fragmentId, GL_SHADER_BINARY_FORMAT_SPIR_V,
                   binary.data(), binary.size() * sizeof(uint32_t));

    glSpecializeShader(ids.fragmentId, "main", 0, nullptr, nullptr);
    if (CheckShaderCompilation(ids.fragmentId) == false)
    {
      return false;
    }
  }
  if (m_shaderPaths.Compute.empty() == false)
  {
    CreateShader(ids.computeId, GL_COMPUTE_SHADER, sourcePath.Compute.c_str());

    const auto binary = LoadShaderBinary(sourcePath.Compute);
    glShaderBinary(1, &ids.computeId, GL_SHADER_BINARY_FORMAT_SPIR_V,
                   binary.data(), binary.size() * sizeof(uint32_t));

    glSpecializeShader(ids.computeId, "main", 0, nullptr, nullptr);
    if (CheckShaderCompilation(ids.computeId) == false)
    {
      return false;
    }
  }
  return true;
}
bool hm::Shader::LoadSource(const ShaderPaths& sourcePath)
{
  static ShaderIds shaderIds {};

  if (GLAD_GL_ARB_gl_spirv && m_usingGLSL == false)
  {
    const auto success = LoadSPIRV(sourcePath, shaderIds);
    if (success == false)
    {
      return false;
    }
  }
  else
  {
    if (CompileGLSL(sourcePath, shaderIds) == false)
    {
      return false;
    }
  }
  // attach shaders
  m_programId = glCreateProgram();

  if (shaderIds.vertexId)
  {
    glAttachShader(m_programId, shaderIds.vertexId);
  }
  if (shaderIds.fragmentId)
  {
    glAttachShader(m_programId, shaderIds.fragmentId);
  }
  if (shaderIds.computeId)
  {
    glAttachShader(m_programId, shaderIds.computeId);
  }

  // link shaders
  if (LinkProgram() == false)
  {
    DeleteShaders(shaderIds);
    log::Error("Shader Linking failed");
    return false;
  }
  DeleteShaders(shaderIds);
  return true;
}
bool hm::Shader::CheckShaderFormatSPIRV(const std::string& path)
{
  if (path.ends_with(".spv"))
  {
    m_usingGLSL = false;
  }
  // returns true if it is .spv
  return m_usingGLSL == false;
}
bool hm::Shader::Load()
{
  ShaderPaths sourcePaths {};
  if (m_shaderPaths.Vertex.empty() == false)
  {
    if (CheckShaderFormatSPIRV(m_shaderPaths.Vertex) == false)
    {
      sourcePaths.Vertex = LoadShader(m_shaderPaths.Vertex);
    }
    else
    {
      sourcePaths.Vertex = m_shaderPaths.Vertex;
    }
  }
  if (m_shaderPaths.Fragment.empty() == false)
  {
    if (CheckShaderFormatSPIRV(m_shaderPaths.Fragment) == false)
    {
      sourcePaths.Fragment = LoadShader(m_shaderPaths.Fragment);
    }
    else
    {
      sourcePaths.Fragment = m_shaderPaths.Fragment;
    }
  }
  if (m_shaderPaths.Compute.empty() == false)
  {
    if (CheckShaderFormatSPIRV(m_shaderPaths.Compute) == false)
    {
      sourcePaths.Compute = LoadShader(m_shaderPaths.Compute);
    }
    else
    {
      sourcePaths.Compute = m_shaderPaths.Compute;
    }
  }

  return LoadSource(sourcePaths);
}
void hm::Shader::Reload()
{
  if (m_programId > 0)
  {
    glDeleteProgram(m_programId);
    m_programId = 0;
  }

  const bool success = Load();

  // TODO

  if (!success)
  {
    log::Error("Unable to load shader {}", m_resourcePath);
  }
}
void hm::Shader::Activate() const
{
  glUseProgram(m_programId);
}
