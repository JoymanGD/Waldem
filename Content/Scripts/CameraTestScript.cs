using Waldem;

namespace Waldem
{
    public class CameraTestScript : ScriptableEntity
    {
        public float ZoomSpeed = 10f;
        
        protected override void OnUpdate(float dt)
        {
            if (Input.IsKeyPressed(KeyCode.R))
            {
                var camera = GetComponent<CameraComponent>();
                camera.FieldOfView += dt * ZoomSpeed;
            }
            if (Input.IsKeyPressed(KeyCode.F))
            {
                var camera = GetComponent<CameraComponent>();
                camera.FieldOfView -= dt * ZoomSpeed;
            }
        }
    }
}