#pragma once
#include <cstdint>
#include <string>

#include "platform/opengl/opengl_gl.hpp"
namespace hm
{

struct ShaderPaths
{
  std::string Vertex {};
  std::string Fragment {};
  std::string Compute {};

  std::string GetPath() const;
};
struct ShaderIds
{
  // TODO add the rest of the ids

  GLuint vertexId {};
  GLuint fragmentId {};
  GLuint computeId {};
};
class Shader
{
 public:
  Shader(const ShaderPaths& shader);

  ~Shader();
  std::string LoadShader(const std::string& path);
  bool CheckShaderCompilation(const GLuint& shaderId);
  bool CreateShader(GLuint& shaderId, GLenum shaderType, const GLchar* source);
  bool CompileShader(GLuint& shaderId, GLenum shaderType, const GLchar* source);
  bool LinkProgram();
  bool CompileGLSL(const hm::ShaderPaths& sourcePath, ShaderIds& ids);
  void DeleteShaders(ShaderIds& ids);
  bool LoadSPIRV(const hm::ShaderPaths& sourcePath, hm::ShaderIds& ids);
  bool LoadSource(const ShaderPaths& sourcePath);
  bool CheckShaderFormatSPIRV(const std::string& path);
  bool Load();
  void Reload();
  void Activate() const;
  uint32_t m_programId {};

 private:
  std::string m_resourcePath {};
  ShaderPaths m_shaderPaths {};
  bool m_usingGLSL {true};
};
} // namespace hm
