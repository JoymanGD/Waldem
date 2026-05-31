namespace Waldem
{
    public sealed class Animator : Component
    {
        public void Play()
        {
            InternalCalls.Animator_Play(EntityId);
        }
        
        public void Stop()
        {
            InternalCalls.Animator_Stop(EntityId);
        }
    }
}
