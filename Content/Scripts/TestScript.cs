using Waldem;

namespace Waldem
{
    public class TestScript : ScriptableEntity
    {
        public float RotationSpeed = 100;
        public float MoveSpeed = 10.0f;

        protected override void OnUpdate(float dt)
        {
            Vector3 movement = Vector3.Zero;

            var animator = GetComponent<AnimatorComponent>();
            animator.Stop();
            
            if (Input.IsKeyPressed(KeyCode.W))
            {
                movement += new Vector3(0, 0, 1);
            }
            if (Input.IsKeyPressed(KeyCode.S))
            {
                movement -= new Vector3(0, 0, 1);
            }
            if (Input.IsKeyPressed(KeyCode.A))
            {
                movement -= new Vector3(1, 0, 0);
            }
            if (Input.IsKeyPressed(KeyCode.D))
            {
                movement += new Vector3(1, 0, 0);
            }
            
            movement.Normalize();

            if (movement.magnitude > 0)
            {
                animator.Play();
            }
            
            Transform.Translate(movement * MoveSpeed * dt);

            if (movement.magnitude > 0)
                Transform.Forward = movement * -1;
            
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