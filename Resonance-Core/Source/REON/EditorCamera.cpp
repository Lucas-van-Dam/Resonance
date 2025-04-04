#include "reonpch.h"
#include "EditorCamera.h"
#include "REON/GameHierarchy/Scene.h"

namespace REON {
    std::shared_ptr<EditorCamera> EditorCamera::GetInstance() {
        static std::shared_ptr<EditorCamera> instance(new EditorCamera());
        return instance;
    }

    EditorCamera::EditorCamera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), ShiftSpeed(SHIFTSPEED), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM), Speed(SPEED)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
        //std::cout << Zoom;
    }

    EditorCamera::EditorCamera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), ShiftSpeed(SHIFTSPEED), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM), Speed(SPEED)
    {
        Position = glm::vec3(posX, posY, posZ);
        WorldUp = glm::vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }


    glm::mat4 EditorCamera::GetProjectionMatrix()
    {
        //   std::cout << "test" << Zoom;
       //    Zoom =45;
        return glm::perspective(glm::radians(Zoom), (float)SceneManager::Get()->GetCurrentScene()->renderManager->GetRenderWidth() / (float)SceneManager::Get()->GetCurrentScene()->renderManager->GetRenderHeight(), 0.1f, 100.0f);
    }

    void EditorCamera::ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = Speed * deltaTime;
        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position += Right * velocity;
        if (direction == RIGHT)
            Position -= Right * velocity;
        if (direction == UP)
            Position += Up * velocity;
        if (direction == DOWN)
            Position -= Up * velocity;
    }

    void EditorCamera::updateCameraVectors()
    {
        // calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = -sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        // also re-calculate the Right and Up vector
        Right = glm::normalize(glm::cross(WorldUp, Front));  // Right-handed cross product order
        Up = glm::normalize(glm::cross(Front, Right));
    }

    void EditorCamera::ProcessShiftKey(bool shift)
    {
        Speed = shift ? ShiftSpeed : MovementSpeed;
    }

    void EditorCamera::ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f;
    }

    void EditorCamera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw -= xoffset;
        Pitch += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

    glm::mat4 EditorCamera::GetViewMatrix() const
    {
        //  return glm::tra();
          //return glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,6) , glm::vec3(0,1,0));
          //std::cout << Position.x << "," << Position.y << "," << Position.z << std::endl;
        return glm::lookAt(Position, Position + Front, Up);
    }

    glm::vec3 EditorCamera::GetPosition() {
        return Position;
    }

    void EditorCamera::SetPosition(glm::vec3 position) {
        Position = position;
    }
}