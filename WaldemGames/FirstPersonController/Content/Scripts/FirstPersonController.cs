using Waldem;

namespace Waldem
{
    public class FirstPersonController : ScriptableEntity
    {
        public float MoveSpeed = 10.0f;
        public float JumpForce = 10.0f;
        public float CameraOrbitSensitivity = 0.2f;
        public float CameraPitchMin = -70.0f;
        public float CameraPitchMax = 75.0f;

        private Vector3 movement;
        private Camera mainCamera;
        private float cameraPitch;

        protected override void OnCreate()
        {
            mainCamera = Camera.Main;
            AddComponent<Animator>();

            if (mainCamera != null)
            {
                cameraPitch = NormalizeAngle(mainCamera.Entity.Transform.Rotation.x);
                cameraPitch = Clamp(cameraPitch, CameraPitchMin, CameraPitchMax);
            }
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
            float cameraVerticalDelta = 0;

            if (Input.GetMouse(MouseButton.Right))
            {
                cameraHorizontal = Input.GetMouseDeltaX() * CameraOrbitSensitivity;
                float targetPitch = cameraPitch + Input.GetMouseDeltaY() * CameraOrbitSensitivity;
                targetPitch = Clamp(targetPitch, CameraPitchMin, CameraPitchMax);
                cameraVerticalDelta = targetPitch - cameraPitch;
                cameraPitch = targetPitch;
            }

            var cameraPos = mainCamera.Entity.Transform.Position;
            mainCamera.Entity.Transform.RotateAround(cameraPos, Vector3.Up, cameraHorizontal);
            mainCamera.Entity.Transform.RotateAround(cameraPos, mainCamera.Entity.Transform.Right, cameraVerticalDelta);
        }

        private float Clamp(float value, float min, float max)
        {
            if (value < min)
                return min;

            if (value > max)
                return max;

            return value;
        }

        private float NormalizeAngle(float angle)
        {
            while (angle > 180.0f)
                angle -= 360.0f;

            while (angle < -180.0f)
                angle += 360.0f;

            return angle;
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
