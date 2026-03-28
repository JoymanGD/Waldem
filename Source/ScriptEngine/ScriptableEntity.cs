namespace Waldem
{
    public abstract class ScriptableEntity : Entity
    {
        protected virtual void OnCreate()
        {
        }

        protected virtual void OnUpdate(float deltaTime)
        {
        }

        protected virtual void OnFixedUpdate(float fixedDeltaTime)
        {
        }

        protected virtual void OnDestroy()
        {
        }
    }
}
