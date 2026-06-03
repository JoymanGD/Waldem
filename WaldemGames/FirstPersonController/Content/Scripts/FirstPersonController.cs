using Waldem;

namespace Waldem
{
    public class FirstPersonController : ScriptableEntity
    {
        public float MoveSpeed = 10.0f;
        public float JumpForce = 10.0f;
        public float CameraOrbitSensitivity = 0.2f;
        public float CameraPitchMin = -20.0f;
        public float CameraPitchMax = 75.0f;

        private Vector3 movement;
        private Camera mainCamera;

        protected override void OnCreate()
        {
            mainCamera = Camera.Main;
        }

        protected override void OnUpdate(float deltaTime)
        {
            movement = Vector3.Zero;

            var animator = GetComponent<Animator>();
            animator?.Stop();

            if (mainCamera != null)
            {
                // UpdateFirstPersonCamera();
                Vector3 forward = mainCamera.Entity.Transform.Forward;
                Vector3 right = mainCamera.Entity.Transform.Right;
                
                forward.y = 0;
                right.y = 0;
                forward.Normalize();
                right.Normalize();

                if (Input.GetKey(KeyCode.W))
                    movement += forward;

                if (Input.GetKey(KeyCode.S))
                    movement -= forward;

                if (Input.GetKey(KeyCode.A))
                    movement -= right;

                if (Input.GetKey(KeyCode.D))
                    movement += right;
                
                movement.Normalize();

                var rigidbody = GetComponent<RigidBody>();

                if (movement.magnitude > 0)
                {
                    if (animator != null)
                    {
                        animator.Play();
                    }
                }

                if (Input.GetKeyDown(KeyCode.Space) && rigidbody.IsGrounded)
                {
                    rigidbody.ApplyImpulse(new Vector3(0, JumpForce, 0));
                }
            }
        }

        private void UpdateFirstPersonCamera()
        {
            if (mainCamera == null)
                return;

            float cameraHorizontal = 0;
            float cameraVertical = 0;
            
            if (Input.GetMouse(MouseButton.Right))
            {
                cameraHorizontal = Input.GetMouseDeltaX() * CameraOrbitSensitivity;
                cameraVertical = Input.GetMouseDeltaY() * CameraOrbitSensitivity;
            }

            var cameraPos = mainCamera.Entity.Transform.Position;
            mainCamera.Entity.Transform.RotateAround(cameraPos, Vector3.Up, cameraHorizontal);
            mainCamera.Entity.Transform.RotateAround(cameraPos, mainCamera.Entity.Transform.Right, cameraVertical);
        }

        protected override void OnFixedUpdate(float fixedDeltaTime)
        {
            var rigidbody = GetComponent<RigidBody>();
            Vector3 velocity = rigidbody.Velocity;
            velocity.x = 0;
            velocity.z = 0;
            
            if (movement.magnitude > 0)
            {
                velocity += movement * MoveSpeed * fixedDeltaTime;
                    
                // rigidbody.RotateTowards(Transform.Position + lastDirection * -1, RotationSpeed * fixedDeltaTime);
            }
            
            rigidbody.Velocity = velocity;
        }

        protected override void OnLateUpdate(float deltaTime)
        {
            if (mainCamera != null)
            {
                UpdateFirstPersonCamera();
            }
        }
    }
}
