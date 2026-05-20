namespace Waldem
{
    public sealed class AnimatorComponent : Component
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
