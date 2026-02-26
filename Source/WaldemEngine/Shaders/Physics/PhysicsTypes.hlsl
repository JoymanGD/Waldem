struct PhysicsData
{
    float3 Velocity;
    float Padding0;
    float3 Acceleration;
    float Padding1;
    float3 Padding2; 
    float Drag;
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

struct AABB
{
    float3 min;
    float3 max;
};

struct BVHNode
{
    AABB Box;
    uint Left;
    uint Right;
    uint ObjectIndex;
};