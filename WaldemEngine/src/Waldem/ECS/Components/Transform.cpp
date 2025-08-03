#include "wdpch.h"
#include "Transform.h"

namespace Waldem
{
    Transform::Transform(Vector3 position)
    {
        Position = position;
        Rotation = { 0, 0, 0 };
        LocalScale = Vector3(1.0f);
        RotationQuat = { 1, 0, 0, 0 };

        Update();
    }

    Transform::Transform(Vector3 position, Quaternion rotation, Vector3 localScale)
    {
        Position = position;
        RotationQuat = rotation;
        LocalScale = localScale;
        Rotation = GetEuler();

        Update();
    }

    Transform::Transform(Matrix4 matrix)
    {
        SetMatrix(matrix);
    }

    void Transform::SetPosition(Vector3 newPosition)
    {
        Position = newPosition;
        
        Update();
    }

    void Transform::SetPosition(float x, float y, float z)
    {
        SetPosition({ x, y, z });
    }

    void Transform::Translate(Vector3 translation)
    {
        Position += translation;

        Update();
    }

    void Transform::Rotate(Quaternion rotation)
    {
        RotationQuat = rotation * RotationQuat;
        Rotation = GetEuler();
        
        Update();
    }

    void Transform::Rotate(float yaw, float pitch, float roll)
    {
        Quaternion verticalRotation = angleAxis(glm::radians(pitch), Vector3(1, 0, 0));
        Quaternion horizontalRotation = angleAxis(glm::radians(yaw), Vector3(0, 1, 0));
        Quaternion rollRotation = angleAxis(glm::radians(roll), Vector3(0, 0, 1));

        RotationQuat = RotationQuat * verticalRotation;
        RotationQuat = horizontalRotation * RotationQuat;
        RotationQuat = rollRotation * RotationQuat;

        Rotation = GetEuler();

        Update();
    }

    void Transform::LookAt(Vector3 target)
    {
        Vector3 direction = normalize(target - Position);
        Vector3 adjustedUp = glm::abs(dot(direction, Vector3(0, 1, 0))) > 0.99f ? Vector3(0, 0, 1) : Vector3(0, 1, 0);
        
        SetRotation(quatLookAt(direction, adjustedUp));
    }

    void Transform::Move(Vector3 delta)
    {
        Vector3 forward = GetForwardVector();
        Vector3 right = GetRightVector();
        Vector3 up = GetUpVector();

        Translate(forward * delta.z + right * delta.x + up * delta.y);
    }

    Vector3 Transform::GetEuler()
    {
        return degrees(eulerAngles(RotationQuat));
    }

    void Transform::SetRotation(float eulerX, float eulerY, float eulerZ)
    {
        Rotation = { eulerX, eulerY, eulerZ };

        SetRotation(Rotation);
    }

    void Transform::SetRotation(Vector3 euler)
    {
        Rotation = euler;
        RotationQuat = Quaternion(radians(Rotation));
        
        Update();
    }

    void Transform::SetRotation(Quaternion newRotation)
    {
        RotationQuat = newRotation;
        Rotation = GetEuler();

        Update();
    }

    void Transform::SetScale(float x, float y, float z)
    {
        SetScale(Vector3(x, y, z));
    }

    void Transform::SetScale(Vector3 localScale)
    {
        LocalScale = localScale;
        
        Update();
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
        RotationQuat = rotation;
        Rotation = GetEuler();
    }

    void Transform::Update()
    {
        //TODO: Optimize this
        Matrix = Matrix4(translate(Matrix4(1.0f), Position) * mat4_cast(RotationQuat) * scale(Matrix4(1.0f), LocalScale));
    }

    void Transform::DecompileMatrix()
    {
        Position = Matrix[3];

        Vector3 scale;
        scale.x = length(Vector3(Matrix[0]));
        scale.y = length(Vector3(Matrix[1]));
        scale.z = length(Vector3(Matrix[2]));
        LocalScale = scale;

        Matrix4 rotationMatrix = Matrix;
        rotationMatrix[0] /= scale.x;
        rotationMatrix[1] /= scale.y;
        rotationMatrix[2] /= scale.z;
        RotationQuat = quat_cast(rotationMatrix);
        Rotation = GetEuler();
    }
}
