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
            animator?.Stop();

            Camera mainCamera = Camera.Main;

            if (mainCamera != null)
            {
                Vector3 forward = mainCamera.Entity.Transform.Forward;
                Vector3 right = mainCamera.Entity.Transform.Right;
                
                forward.y = 0;
                right.y = 0;
                forward.Normalize();
                right.Normalize();

                if (Input.IsKeyPressed(KeyCode.W))
                    movement += forward;

                if (Input.IsKeyPressed(KeyCode.S))
                    movement -= forward;

                if (Input.IsKeyPressed(KeyCode.A))
                    movement -= right;

                if (Input.IsKeyPressed(KeyCode.D))
                    movement += right;
                
                movement.Normalize();

                if (movement.magnitude > 0)
                {
                    if (animator != null)
                    {
                        animator.Play();
                    }
                }
                
                Transform.Translate(movement * MoveSpeed * dt);

                if (movement.magnitude > 0)
                    Transform.Forward = movement * -1;
            }
        }

        protected override void OnCollisionEnter(Collision collision)
        {
            Debug.Log("Collision enter!");
        }
    }
}