#pragma once

namespace Waldem
{
    struct PhysicsData
    {
        Vector3 Velocity;
        float Padding0;
        Vector3 Acceleration = { 2, 2, 2 };
        float Padding1;
        Vector3 Padding2; 
        float Drag = 3.0f; //size 48
    };
    
    struct PhysicsComponent
    {
        PhysicsData Linear;
        PhysicsData Angular;
        float Mass;
        uint IsKinematic;
        uint IsGravity;
        uint Padding0;
    };
}