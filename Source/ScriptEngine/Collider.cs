using System;

namespace Waldem
{
    public struct ContactPoint
    {
        public Vector3 Position;
        public Vector3 PositionA;
        public Vector3 PositionB;
        public Vector3 Normal;
        public float Penetration;
        public ulong OtherEntityId;
    }

    public struct Collision
    {
        public ContactPoint Contact;
        public Entity Other;
    }
    
    public sealed class Collider : Component
    {
        
    }
}
