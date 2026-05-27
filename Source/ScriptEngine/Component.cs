namespace Waldem
{
    public abstract class Component
    {
        internal ulong EntityId;

        internal void Attach(ulong entityId)
        {
            EntityId = entityId;
        }

        public Entity Entity
        {
            get
            {
                Entity entity = new Entity();
                entity.__SetEntityId(EntityId);
                return entity;
            }
        }
    }
}
