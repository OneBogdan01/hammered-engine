#include "camera.hpp"

#include "engine.hpp"
#include "core/device.hpp"
#include "utility/logger.hpp"

#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

void Camera::Update(f32)
{
  glm::mat4 cameraRotation = getRotationMatrix();
  position += glm::vec3(cameraRotation * glm::vec4(velocity * 0.5f, 0.f));
}
void Camera::Render() {}
Camera::~Camera() {}
void Camera::HandleInput(SDL_Event* event)
{
  SDL_Window* window = &hm::Engine::Instance().GetDevice().GetSDLWindow();

  if (event->type == SDL_EVENT_KEY_DOWN)
  {
    if (event->key.scancode == SDL_SCANCODE_W)
    {
      velocity.z = -1;
    }
    if (event->key.scancode == SDL_SCANCODE_S)
    {
      velocity.z = 1;
    }
    if (event->key.scancode == SDL_SCANCODE_A)
    {
      velocity.x = -1;
    }
    if (event->key.scancode == SDL_SCANCODE_D)
    {
      velocity.x = 1;
    }

    // changes mode
    if (event->key.scancode == SDL_SCANCODE_SPACE && event->key.repeat == false)
    {
      SDL_SetWindowRelativeMouseMode(window,
                                     !SDL_GetWindowRelativeMouseMode(window));
    }
  }

  if (event->type == SDL_EVENT_KEY_UP)
  {
    if (event->key.scancode == SDL_SCANCODE_W)
    {
      velocity.z = 0;
    }
    if (event->key.scancode == SDL_SCANCODE_S)
    {
      velocity.z = 0;
    }
    if (event->key.scancode == SDL_SCANCODE_A)
    {
      velocity.x = 0;
    }
    if (event->key.scancode == SDL_SCANCODE_D)
    {
      velocity.x = 0;
    }
  }

  if (event->type == SDL_EVENT_MOUSE_MOTION &&
      SDL_GetWindowRelativeMouseMode(window) == true)
  {
    yaw += event->motion.xrel / 200.f;
    pitch -= event->motion.yrel / 200.f;
  }
}

Camera::Camera(const std::string& name) : System(name) {}
glm::mat4 Camera::getViewMatrix() const
{
  // to create a correct model view, we need to move the world in opposite
  // direction to the camera
  //  so we will create the camera model matrix and invert
  glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), position);
  glm::mat4 cameraRotation = getRotationMatrix();
  return glm::inverse(cameraTranslation * cameraRotation);
}

glm::mat4 Camera::getRotationMatrix() const
{
  // fairly typical FPS style camera. we join the pitch and yaw rotations into
  // the final rotation matrix

  glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3 {1.f, 0.f, 0.f});
  glm::quat yawRotation = glm::angleAxis(yaw, glm::vec3 {0.f, -1.f, 0.f});

  return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
}
