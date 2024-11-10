#include "wdpch.h"
#include "Transform.h"

namespace Waldem
{
    Transform::Transform(Vector3 position)
    {
        Position = position;
        Rotation = { 1, 0, 0, 0 };
        LocalScale = Vector3(1.0f);

        CompileMatrix();
    }

    Transform::Transform(Vector3 position, Quaternion rotation, Vector3 localScale)
    {
        Position = position;
        Rotation = rotation;
        LocalScale = localScale;

        CompileMatrix();
    }

    Transform::Transform(Matrix4 matrix)
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

    void Transform::SetPosition(Vector3 newPosition)
    {
        Position = newPosition;
        
        CompileMatrix();
    }

    void Transform::SetPosition(float x, float y, float z)
    {
        SetPosition({ x, y, z });
    }

    void Transform::Translate(Vector3 translation)
    {
        Position += translation;

        CompileMatrix();
    }

    void Transform::Rotate(Quaternion rotation)
    {
        Rotation = rotation * Rotation;
        
        CompileMatrix();
    }

    void Transform::Rotate(float yaw, float pitch, float roll)
    {
        Quaternion rotation = Quaternion(Vector3(pitch, yaw, roll));

        Rotate(rotation);
    }

    void Transform::SetEuler(Vector3 euler)
    {
        euler = radians(euler);
        Rotation = Quaternion(euler);
        
        CompileMatrix();
    }

    void Transform::SetEuler(float eulerX, float eulerY, float eulerZ)
    {
        Vector3 euler = { eulerX, eulerY, eulerZ };

        SetEuler(euler);
    }

    void Transform::SetRotation(Quaternion newRotation)
    {
        Rotation = newRotation;

        CompileMatrix();
    }

    void Transform::Scale(Vector3 localScale)
    {
        LocalScale = localScale;
        
        CompileMatrix();
    }

    void Transform::SetMatrix(Matrix4 matrix)
    {
        Matrix = Matrix4(matrix);
        
        Position = matrix[3];
        
        Vector3 scale;
        scale.x = length(Vector3(matrix[0]));
        scale.y = length(Vector3(matrix[1]));
        scale.z = length(Vector3(matrix[2]));
        LocalScale = scale;

        Matrix4 rotationMatrix = matrix;
        rotationMatrix[0] /= scale.x;
        rotationMatrix[1] /= scale.y;
        rotationMatrix[2] /= scale.z;
        Quaternion rotation = quat_cast(rotationMatrix);
        Rotation = rotation;
    }

    Matrix4 Transform::Inverse()
    {
        return inverse(Matrix);
    }

    void Transform::CompileMatrix()
    {
        Matrix = Matrix4(translate(Matrix4(1.0f), Position) * mat4_cast(Rotation) * scale(Matrix4(1.0f), LocalScale));
    }
}
