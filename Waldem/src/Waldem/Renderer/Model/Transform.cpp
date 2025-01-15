#include "wdpch.h"
#include "Transform.h"

namespace Waldem
{
#define DEGTORAD 0.0174532925199432957f
    
    Transform::Transform(Vector3 position)
    {
        Position = position;
        Rotation = { 1, 0, 0, 0 };
        LocalScale = Vector3(1.0f);

        Update();
    }

    Transform::Transform(Vector3 position, Quaternion rotation, Vector3 localScale)
    {
        Position = position;
        Rotation = rotation;
        LocalScale = localScale;

        Update();
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
        
        Update();
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
        Rotation = rotation * Rotation;
        
        Update();
    }

    void Transform::Rotate(float yaw, float pitch, float roll)
    {
        Quaternion verticalRotation = angleAxis(pitch * DEGTORAD, Vector3(1, 0, 0));
        Quaternion horizontalRotation = angleAxis(yaw * DEGTORAD, Vector3(0, 1, 0));
        Quaternion rollRotation = angleAxis(roll * DEGTORAD, Vector3(0, 0, 1));

        Rotation = Rotation * verticalRotation;
        Rotation = horizontalRotation * Rotation;
        Rotation = rollRotation * Rotation;

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

    void Transform::SetEuler(Vector3 euler)
    {
        euler = radians(euler);
        Rotation = Quaternion(euler);
        
        Update();
    }

    void Transform::SetEuler(float eulerX, float eulerY, float eulerZ)
    {
        Vector3 euler = { eulerX, eulerY, eulerZ };

        SetEuler(euler);
    }

    void Transform::SetRotation(Quaternion newRotation)
    {
        Rotation = newRotation;

        Update();
    }

    void Transform::Scale(Vector3 localScale)
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
        Rotation = rotation;
    }

    void Transform::Update()
    {
        //TODO: Optimize this
        Matrix = Matrix4(translate(Matrix4(1.0f), Position) * mat4_cast(Rotation) * scale(Matrix4(1.0f), LocalScale));
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
        Rotation = quat_cast(rotationMatrix);
    }
}
