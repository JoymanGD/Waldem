#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

namespace Waldem
{
    class WALDEM_API Transform
    {
    public:
        Transform() {}
        Transform(glm::vec3 position);
        Transform(glm::vec3 position, glm::quat rotation, glm::vec3 localScale);
        Transform(glm::mat4 matrix);
        
        glm::mat4 GetMatrix() { return Matrix; }
        glm::vec3 GetPosition() { return Position; }
        glm::quat GetRotation() { return Rotation; }
        glm::vec3 GetLocalScale() { return LocalScale; }
        glm::vec3 GetForwardVector() { return glm::vec3(Matrix * glm::vec4(0, 0, -1, 0)); }
        glm::vec3 GetRightVector() { return glm::vec3(Matrix * glm::vec4(1, 0, 0, 0)); }
        glm::vec3 GetUpVector() { return glm::vec3(Matrix * glm::vec4(0, 1, 0, 0)); }
        
        void Reset();
        void SetPosition(glm::vec3 newPosition);
        void SetPosition(float x, float y, float z);
        void Translate(glm::vec3 translation);
        void Rotate(glm::quat rotation);
        void SetRotation(glm::quat newRotation);
        void Scale(glm::vec3 scale);
        void SetMatrix(glm::mat4 matrix);
        glm::mat4 Inverse();
    private:
        void CompileMatrix();
        
        glm::vec3 Position;
        glm::quat Rotation;
        glm::vec3 LocalScale;
        glm::mat4 Matrix;
        glm::mat4 InversedMatrix;
    };
}
