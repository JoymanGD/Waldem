#pragma once

#include "ComponentBase.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

namespace Waldem
{
    struct WALDEM_API Transform
    {
        COMPONENT(Transform)
            FIELD(Vector3, Position)
            FIELD(Vector3, Rotation)
            FIELD(Vector3, LocalScale)
        END_COMPONENT()
        
        Vector3 Position = { 0, 0, 0 };
        Vector3 Rotation = { 0, 0, 0 }; 
        Vector3 LocalScale = { 1, 1, 1 };
        Matrix4 Matrix = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
        Quaternion RotationQuat = { 1, 0, 0, 0 };

        Transform() = default;
        Transform(Vector3 position);
        Transform(Vector3 position, Quaternion rotation, Vector3 localScale);
        Transform(Matrix4 matrix);
        
        Vector3 GetForwardVector() const { return Vector3(Matrix * Vector4(0, 0, 1, 0)); }
        Vector3 GetRightVector() const { return Vector3(Matrix * Vector4(1, 0, 0, 0)); }
        Vector3 GetUpVector() const { return Vector3(Matrix * Vector4(0, 1, 0, 0)); }

        operator Matrix4() const { return Matrix; }
        
        void SetPosition(Vector3 newPosition); 
        void SetPosition(float x, float y, float z);
        void Translate(Vector3 translation);
        void Rotate(Quaternion rotation);
        void Rotate(float yaw, float pitch, float roll);
        void LookAt(Vector3 target);
        void Move(Vector3 delta);
        void SetRotation(Vector3 pitchYawRoll);
        void SetRotation(float pitch, float yaw, float roll);
        void SetRotation(Quaternion newRotation);
        void SetScale(float x, float y, float z);
        void SetScale(Vector3 scale);
        void SetMatrix(Matrix4 matrix);
        Matrix4 Inverse() { return inverse(Matrix); }
        void Update();
        void DecompileMatrix();

        Matrix3x4 ToMatrix3x4() const
        {
            Matrix4 originalMat = transpose(Matrix);
            Matrix3x4 mat3x4;
            
            for (int row = 0; row < 3; ++row)
            {
                for (int col = 0; col < 4; ++col)
                {
                    mat3x4[row][col] = originalMat[row][col];
                }
            }
            
            return mat3x4;
        }
        
    private:
        void ClampRotation();
    };
}