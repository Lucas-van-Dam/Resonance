#pragma once

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include "REON/Platform/Windows/WindowsInput.h"
#include "REON/GameHierarchy/SceneManager.h"
#include <REON/GameHierarchy/Components/Camera.h>

namespace REON {

    class Scene;

    // Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
    enum Camera_Movement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN
    };

    // Default camera values
    const float YAW = -90.0f;
    const float PITCH = 0.0f;
    const float SPEED = 2.5f;
    const float SHIFTSPEED = 5.0f;
    const float SENSITIVITY = 0.3f;
    const float ZOOM = 45.0f;


    // An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
    class EditorCamera : public Camera
    {
    public:
        static std::shared_ptr<EditorCamera> GetInstance();

        // constructor with vectors
        EditorCamera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH);

        // constructor with scalar values
        EditorCamera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);

        void SetPosition(glm::vec3 position);

        glm::vec3 GetPosition();

        // returns the view matrix calculated using Euler Angles and the LookAt Matrix
        glm::mat4 GetViewMatrix() const override;

        glm::mat4 GetProjectionMatrix() const override;

        // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
        void ProcessKeyboard(Camera_Movement direction, float deltaTime);

        // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
        void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

        // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
        void ProcessMouseScroll(float yoffset);

        void ProcessShiftKey(bool shift);

    public:
        // camera Attributes
        glm::vec3 Position;
        glm::vec3 Front;
        glm::vec3 Up;
        glm::vec3 Right;
        glm::vec3 WorldUp;
        // euler Angles
        float Yaw;
        float Pitch;
        // camera options
        float MovementSpeed;
        float ShiftSpeed;
        float Speed;
        float MouseSensitivity;
        float Zoom;

        float zNear = 0.1f;
        float zFar = 100.0f;

    private:
        // calculates the front vector from the Camera's (updated) Euler Angles
        void updateCameraVectors();
    };

}

