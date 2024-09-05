#include "wdpch.h"
#include "Transform.h"

namespace Waldem
{
    Transform::Transform(glm::vec3 position)
    {
        Position = position;
        Rotation = { 1, 0, 0, 0 };
        LocalScale = glm::vec3(1.0f);

        CompileMatrix();
    }

    Transform::Transform(glm::vec3 position, glm::quat rotation, glm::vec3 localScale)
    {
        Position = position;
        Rotation = rotation;
        LocalScale = localScale;

        CompileMatrix();
    }

    Transform::Transform(glm::mat4 matrix)
    {
        SetMatrix(matrix);
    }

    void Transform::Reset()
    {
        LocalScale = { 1, 1, 1 };
        Position = { 0, 0, 0 };
        Rotation = { 1, 0, 0, 0 };
        
        CompileMatrix();
    }

    void Transform::SetPosition(glm::vec3 newPosition)
    {
        Position = newPosition;
        
        CompileMatrix();
    }

    void Transform::Translate(glm::vec3 translation)
    {
        Position += translation;

        CompileMatrix();
    }

    void Transform::Rotate(glm::quat rotation)
    {
        Rotation = rotation * Rotation;
        
        CompileMatrix();
    }

    void Transform::SetRotation(glm::quat newRotation)
    {
        Rotation = newRotation;

        CompileMatrix();
    }

    void Transform::Scale(glm::vec3 localScale)
    {
        LocalScale = localScale;
        
        CompileMatrix();
    }

    void Transform::SetMatrix(glm::mat4 matrix)
    {
        Matrix = glm::mat4(matrix);
        
        Position = matrix[3];
        
        glm::vec3 scale;
        scale.x = length(glm::vec3(matrix[0]));
        scale.y = length(glm::vec3(matrix[1]));
        scale.z = length(glm::vec3(matrix[2]));
        LocalScale = scale;

        glm::mat4 rotationMatrix = matrix;
        rotationMatrix[0] /= scale.x;
        rotationMatrix[1] /= scale.y;
        rotationMatrix[2] /= scale.z;
        glm::quat rotation = glm::quat_cast(rotationMatrix);
        Rotation = rotation;
    }

    glm::mat4 Transform::Inverse()
    {
        return inverse(Matrix);
    }

    void Transform::CompileMatrix()
    {
        Matrix = glm::mat4(translate(glm::mat4(1.0f), Position) * mat4_cast(Rotation) * scale(glm::mat4(1.0f), LocalScale));
    }
}
