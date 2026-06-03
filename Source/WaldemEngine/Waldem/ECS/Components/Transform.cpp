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
        LocalScale = localScale;
        SetRotation(rotation);

        Update();
    }

    Transform::Transform(Matrix4 matrix)
    {
        SetMatrix(matrix);
    }

    Vector3 Transform::SetForwardVector(Vector3 forwardVector)
    {
        float len = glm::length(forwardVector);
        if (len < 1e-6f)
            return GetForwardVector();
        forwardVector /= len;
        Vector3 adjustedUp = glm::abs(dot(forwardVector, Vector3(0, 1, 0))) > 0.99f ? Vector3(0, 0, 1) : Vector3(0, 1, 0);
        SetRotation(quatLookAt(forwardVector, adjustedUp));
        return forwardVector;
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
        RotationQuat = normalize(rotation * RotationQuat);
        Rotation = degrees(eulerAngles(RotationQuat));
        LastRotation = Rotation;
        
        Update();
    }

    void Transform::Rotate(float pitch, float yaw, float roll)
    {
        Quaternion verticalRotation = angleAxis(glm::radians(pitch), Vector3(1, 0, 0));
        Quaternion horizontalRotation = angleAxis(glm::radians(yaw), Vector3(0, 1, 0));
        Quaternion rollRotation = angleAxis(glm::radians(roll), Vector3(0, 0, 1));

        RotationQuat = RotationQuat * verticalRotation;
        RotationQuat = horizontalRotation * RotationQuat;
        RotationQuat = rollRotation * RotationQuat;
        
        Vector3 pitchYawRoll = Vector3(pitch, yaw, roll);

        Rotation += pitchYawRoll;
        LastRotation = Rotation;

        Update();
    }

    void Transform::LookAt(Vector3 target)
    {
        Vector3 direction = normalize(target - Position);
        Vector3 adjustedUp = glm::abs(dot(direction, Vector3(0, 1, 0))) > 0.99f ? Vector3(0, 0, 1) : Vector3(0, 1, 0);
        
        SetRotation(quatLookAt(direction, adjustedUp));
    }

    void Transform::LookAt(Vector3 target, Vector3 up)
    {
        Vector3 direction = target - Position;
        if(length(direction) < 1e-6f)
        {
            return;
        }

        if(length(up) < 1e-6f)
        {
            up = Vector3(0, 1, 0);
        }

        direction = normalize(direction);
        up = normalize(up);

        if(glm::abs(dot(direction, up)) > 0.99f)
        {
            up = glm::abs(dot(direction, Vector3(0, 1, 0))) > 0.99f ? Vector3(0, 0, 1) : Vector3(0, 1, 0);
        }

        SetRotation(quatLookAt(direction, up));
    }

    void Transform::RotateAround(Vector3 point, Vector3 axis, float angleDegrees)
    {
        if(length(axis) < 1e-6f)
        {
            return;
        }

        Quaternion rotation = angleAxis(glm::radians(angleDegrees), normalize(axis));
        Position = point + (rotation * (Position - point));
        RotationQuat = normalize(rotation * RotationQuat);
        Rotation = degrees(eulerAngles(RotationQuat));
        LastRotation = Rotation;

        Update();
    }

    void Transform::Move(Vector3 delta, TransformSpace space)
    {
        Vector3 movement = delta;
        
        if(space == Local)
        {
            Vector3 forward = GetForwardVector();
            Vector3 right = GetRightVector();
            Vector3 up = GetUpVector();

            movement = forward * delta.z + right * delta.x + up * delta.y;
        }
        
        Translate(movement);
    }

    void Transform::SetRotation(float pitch, float yaw, float roll)
    {
        SetRotation(Vector3(pitch, yaw, roll));
    }

    void Transform::SetRotation(Vector3 pitchYawRoll)
    {
        Rotation = pitchYawRoll;
        
        ApplyPitchYawRoll();
        LastRotation = Rotation;
        Update();
    }

    void Transform::SetRotation(Quaternion newRotation)
    {
        RotationQuat = normalize(newRotation);
        Rotation = degrees(eulerAngles(RotationQuat));
        LastRotation = Rotation;

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
        SetRotation(rotation);
    }

    void Transform::Update()
    {
        // ClampRotation();

        //TODO: Optimize this
        Matrix = Matrix4(translate(Matrix4(1.0f), Position) * mat4_cast(RotationQuat) * scale(Matrix4(1.0f), LocalScale));
        SyncRenderStateFromSimulation();
    }

    void Transform::ApplyPitchYawRoll()
    {
        auto deltaRotation = Rotation - LastRotation;
        Quaternion deltaQuat = Quaternion(radians(deltaRotation));
        // RotationQuat = Quaternion(radians(Rotation));
        RotationQuat = deltaQuat * RotationQuat;
    }

    void Transform::ResetQuaternion()
    {
        Vector3 r = radians(Rotation);

        // match your Rotate(float yaw, pitch, roll) implementation
        Quaternion qx = angleAxis(r.x, Vector3(1,0,0));
        Quaternion qy = angleAxis(r.y, Vector3(0,1,0));
        Quaternion qz = angleAxis(r.z, Vector3(0,0,1));

        // choose the order consistently (say, Z * Y * X)
        RotationQuat = qz * qy * qx;
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

        auto lastRotationQuat = RotationQuat;
        RotationQuat = quat_cast(rotationMatrix);

        auto deltaQuat = RotationQuat * inverse(lastRotationQuat);
        
        auto pitch = glm::pitch(deltaQuat);
        auto yaw = glm::yaw(deltaQuat);
        auto roll = glm::roll(deltaQuat);
        Rotation += degrees(Vector3(pitch, yaw, roll));
        // ClampRotation();
        LastRotation = Rotation;
        SyncRenderStateFromSimulation();
    }

    void Transform::SyncRenderStateFromSimulation()
    {
        RenderMatrix = Matrix;
        RenderPosition = Position;
        RenderScale = LocalScale;
        RenderRotationQuat = RotationQuat;
    }

    void Transform::SetRenderMatrix(const Matrix4& matrix)
    {
        RenderMatrix = matrix;
        RenderPosition = matrix[3];

        Vector3 scale;
        scale.x = length(Vector3(matrix[0]));
        scale.y = length(Vector3(matrix[1]));
        scale.z = length(Vector3(matrix[2]));
        RenderScale = scale;

        Matrix4 rotationMatrix = matrix;
        if(scale.x > 1e-6f) rotationMatrix[0] /= scale.x;
        if(scale.y > 1e-6f) rotationMatrix[1] /= scale.y;
        if(scale.z > 1e-6f) rotationMatrix[2] /= scale.z;
        RenderRotationQuat = quat_cast(rotationMatrix);
    }

    void Transform::InitializePhysicsInterpolationState()
    {
        PhysicsPreviousPosition = Position;
        PhysicsCurrentPosition = Position;
        PhysicsPreviousRotationQuat = RotationQuat;
        PhysicsCurrentRotationQuat = RotationQuat;
        HasPhysicsInterpolationState = true;
    }

    void Transform::PushPhysicsInterpolationState(const Vector3& position, const Quaternion& rotation)
    {
        if(!HasPhysicsInterpolationState)
        {
            InitializePhysicsInterpolationState();
        }

        PhysicsPreviousPosition = PhysicsCurrentPosition;
        PhysicsPreviousRotationQuat = PhysicsCurrentRotationQuat;
        PhysicsCurrentPosition = position;
        PhysicsCurrentRotationQuat = normalize(rotation);
    }

    void Transform::ApplyPhysicsInterpolation(float alpha)
    {
        if(!HasPhysicsInterpolationState)
        {
            SyncRenderStateFromSimulation();
            return;
        }

        alpha = glm::clamp(alpha, 0.0f, 1.0f);
        RenderPosition = glm::mix(PhysicsPreviousPosition, PhysicsCurrentPosition, alpha);
        RenderRotationQuat = normalize(glm::slerp(PhysicsPreviousRotationQuat, PhysicsCurrentRotationQuat, alpha));
        RenderScale = LocalScale;
        RenderMatrix = translate(Matrix4(1.0f), RenderPosition) * mat4_cast(RenderRotationQuat) * scale(Matrix4(1.0f), RenderScale);
    }
    
    void Transform::ClampRotation()
    {
        Rotation.x = fmod(Rotation.x + 180.0f, 360.0f);
        if (Rotation.x < 0.0f) Rotation.x += 360.0f;
        Rotation.x -= 180.0f;

        Rotation.y = fmod(Rotation.y + 180.0f, 360.0f);
        if (Rotation.y < 0.0f) Rotation.y += 360.0f;
        Rotation.y -= 180.0f;

        Rotation.z = fmod(Rotation.z + 180.0f, 360.0f);
        if (Rotation.z < 0.0f) Rotation.z += 360.0f;
        Rotation.z -= 180.0f;
    }
}
