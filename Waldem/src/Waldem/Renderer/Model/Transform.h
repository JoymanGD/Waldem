#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

namespace Waldem
{
    struct WALDEM_API Transform
    {
        Transform(Vector3 position);
        Transform(Vector3 position, Quaternion rotation, Vector3 localScale);
        Transform(Matrix4 matrix);
        
        Vector3 GetPosition() { return Position; }
        Quaternion GetRotation() { return Rotation; }
        Vector3 GetLocalScale() { return LocalScale; }
        Vector3 GetForwardVector() { return Vector3(Matrix * Vector4(0, 0, 1, 0)); }
        Vector3 GetRightVector() { return Vector3(Matrix * Vector4(1, 0, 0, 0)); }
        Vector3 GetUpVector() { return Vector3(Matrix * Vector4(0, 1, 0, 0)); }

        operator Matrix4() const { return Matrix; }
        
        void Reset();
        void SetPosition(Vector3 newPosition); 
        void SetPosition(float x, float y, float z);
        void Translate(Vector3 translation);
        void Rotate(Quaternion rotation);
        void Rotate(float yaw, float pitch, float roll);
        void LookAt(Vector3 target);
        void Move(Vector3 delta);
        void SetEuler(Vector3 euler);
        void SetEuler(float eulerX, float eulerY, float eulerZ);
        void SetRotation(Quaternion newRotation);
        void Scale(Vector3 scale);
        void SetMatrix(Matrix4 matrix);
        Matrix4 Inverse();
        Matrix4 Matrix;
    private:
        void CompileMatrix();
        
        Vector3 Position;
        Quaternion Rotation;
        Vector3 LocalScale;
    };
}
