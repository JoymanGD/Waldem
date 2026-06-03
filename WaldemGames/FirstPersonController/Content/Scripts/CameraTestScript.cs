using Waldem;

namespace Waldem
{
    public class CameraTestScript : ScriptableEntity
    {
        public float ZoomSpeed = 10f;
        
        protected override void OnUpdate(float dt)
        {
            if (Input.GetKey(KeyCode.R))
            {
                var camera = GetComponent<Camera>();
                camera.FieldOfView += dt * ZoomSpeed;
            }
            if (Input.GetKey(KeyCode.F))
            {
                var camera = GetComponent<Camera>();
                camera.FieldOfView -= dt * ZoomSpeed;
            }
            
            var transform = GetComponent<Transform>();
            transform.Forward = new Vector3(0, 0, 1);
        }
    }
}