namespace Waldem
{
    public abstract class Component
    {
        internal ulong EntityId;

        internal void Attach(ulong entityId)
        {
            EntityId = entityId;
        }
    }
}
