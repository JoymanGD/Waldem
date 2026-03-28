using Waldem;

namespace Waldem
{
    public class RotateOverTime : ScriptableEntity
    {
        public float RotationSpeed = 100;
        public float MoveSpeed = 10.0f;

        protected override void OnUpdate(float dt)
        {
            Vector3 movement = Vector3.Zero;
            
            if (Input.IsKeyPressed(KeyCode.W))
            {
                movement += Transform.Up;
            }
            if (Input.IsKeyPressed(KeyCode.S))
            {
                movement -= Transform.Up;
            }
            if (Input.IsKeyPressed(KeyCode.A))
            {
                movement -= Transform.Right;
            }
            if (Input.IsKeyPressed(KeyCode.D))
            {
                movement += Transform.Right;
            }
            
            movement.Normalize();
            
            Transform.Translate(movement * MoveSpeed * dt);
            
            Vector3 rotation = Vector3.Zero;

            if (Input.IsKeyPressed(KeyCode.E))
            {
                rotation += new Vector3(1, 0, 0);
            }
            if (Input.IsKeyPressed(KeyCode.Q))
            {
                rotation -= new Vector3(1, 0, 0);
            }
            
            Transform.Rotate(rotation * RotationSpeed * dt);
        }
    }
}