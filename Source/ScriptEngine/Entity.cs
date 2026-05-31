using System;

namespace Waldem
{
    internal enum ComponentKind
    {
        None = 0,
        Transform = 1,
        Camera = 2,
        RigidBody = 3,
        Light = 4,
        Animator = 5
    }

    public class Entity
    {
        internal ulong EntityId;

        public ulong Id => EntityId;

        internal void __SetEntityId(ulong entityId)
        {
            EntityId = entityId;
        }

        public bool HasComponent<T>() where T : Component, new()
        {
            return InternalCalls.Entity_HasComponent(EntityId, (int)GetComponentKind(typeof(T)));
        }

        public T GetComponent<T>() where T : Component, new()
        {
            if (!HasComponent<T>())
                return null;

            T component = new T();
            component.Attach(EntityId);
            return component;
        }

        public void Destroy()
        {
            InternalCalls.Entity_Destroy(EntityId);
        }

        public Transform Transform => GetComponent<Transform>();
        public Camera Camera => GetComponent<Camera>();
        public RigidBody RigidBody => GetComponent<RigidBody>();
        public Light Light => GetComponent<Light>();
        public Animator Animator => GetComponent<Animator>();

        private static ComponentKind GetComponentKind(Type type)
        {
            if (type == typeof(Transform)) return ComponentKind.Transform;
            if (type == typeof(Camera)) return ComponentKind.Camera;
            if (type == typeof(RigidBody)) return ComponentKind.RigidBody;
            if (type == typeof(Light)) return ComponentKind.Light;
            if (type == typeof(Animator)) return ComponentKind.Animator;

            return ComponentKind.None;
        }
    }
}
