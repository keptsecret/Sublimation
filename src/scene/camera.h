#pragma once

#include <glm/glm.hpp>

namespace sublimation {

enum class CameraMovement {
    Forward,
    Backward,
    Right,
    Left
};

class Camera {
public:
    Camera(const glm::vec3& pos = { 0, 0, 0 }, const glm::vec3& up = { 0, 1, 0 }, float yaw = -90.f, float pitch = 0.0f);

    glm::mat4 getProjectionTransform() const;
    glm::mat4 getViewTransform() const;

    void translate(CameraMovement direction, float delta_time);
    void pan(float xoffset, float yoffset, bool constrain_pitch = true);
    void updateViewportSize(uint32_t w, uint32_t h) {
        width = w;
        height = h;
    }

    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 world_up;

    float yaw;
    float pitch;

    float speed;
    float sensitivity;
    float fov; ///< represents vertical fov, default = 50deg

    float near_plane;
    float far_plane;

private:
    void updateCameraVectors();

    uint32_t width = 1920, height = 1080;
};

} //namespace sublimation