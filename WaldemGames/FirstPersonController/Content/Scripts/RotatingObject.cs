using Waldem;

namespace Waldem
{
    public class RotatingObject : ScriptableEntity
    {
        public Vector3 RotationSpeed = new Vector3(30, 0, 0);

        protected override void OnUpdate(float dt)
        {
            Transform.Rotate(RotationSpeed * dt);
        }
    }
}