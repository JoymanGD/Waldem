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
        Transform(Vector3 position);
        Transform(Vector3 position, Quaternion rotation, Vector3 localScale);
        Transform(Matrix4 matrix);
        
        Matrix4 GetMatrix() { return Matrix; }
        Vector3 GetPosition() { return Position; }
        Quaternion GetRotation() { return Rotation; }
        Vector3 GetLocalScale() { return LocalScale; }
        Vector3 GetForwardVector() { return Vector3(Matrix * Vector4(0, 0, -1, 0)); }
        Vector3 GetRightVector() { return Vector3(Matrix * Vector4(1, 0, 0, 0)); }
        Vector3 GetUpVector() { return Vector3(Matrix * Vector4(0, 1, 0, 0)); }

        operator Matrix4() const { return Matrix; }
        
        void Reset();
        void SetPosition(Vector3 newPosition);
        void SetPosition(float x, float y, float z);
        void Translate(Vector3 translation);
        void Rotate(Quaternion rotation);
        void SetRotation(Quaternion newRotation);
        void Scale(Vector3 scale);
        void SetMatrix(Matrix4 matrix);
        Matrix4 Inverse();
    private:
        void CompileMatrix();
        
        Vector3 Position;
        Quaternion Rotation;
        Vector3 LocalScale;
        Matrix4 Matrix;
        Matrix4 InversedMatrix;
    };
}
