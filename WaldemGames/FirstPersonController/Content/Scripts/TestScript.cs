namespace Waldem
{
    public class TestScript : ScriptableEntity
    {
        public float RotationSpeed = 100;
        public float MoveSpeed = 10.0f;
        public float JumpForce = 10.0f;
        public Vector3 CameraOffset = new Vector3(0, 2, -2);
        public float CameraFollowSpeed = .8f;
        public float CameraOrbitSensitivity = 0.2f;
        public float CameraPitchMin = -20.0f;
        public float CameraPitchMax = 75.0f;

        private Vector3 movement;
        private Vector3 lastDirection;
        private Camera mainCamera;
        private float cameraYaw;
        private float cameraPitch;
        private float cameraDistance;

        protected override void OnCreate()
        {
            lastDirection = Transform.Forward;
            mainCamera = Camera.Main;

            if (mainCamera != null)
            {
                Vector3 offset = mainCamera.Entity.Transform.Position - Transform.Position;
                cameraDistance = offset.magnitudeF;

                float horizontalLength = Math.Sqrt(offset.x * offset.x + offset.z * offset.z);
                cameraYaw = Math.Atan2(offset.x, offset.z) * Math.Rad2Deg;
                cameraPitch = Math.Atan2(offset.y, Math.Max(horizontalLength, 0.0001f)) * Math.Rad2Deg;
            }
            else
            {
                cameraDistance = CameraOffset.magnitudeF;
            }
        }

        protected override void OnUpdate(float dt)
        {
            movement = Vector3.Zero;

            var animator = GetComponent<Animator>();
            animator?.Stop();

            if (mainCamera != null)
            {
                UpdateOrbitalCamera(dt);

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
                    lastDirection = movement;
                    
                    if (animator != null)
                    {
                        animator.Play();
                    }
                }

                if (Input.GetKeyDown(KeyCode.Space))
                {
                    rigidbody.ApplyImpulse(new Vector3(0, JumpForce, 0));
                }
            }
        }

        private void UpdateOrbitalCamera(float dt)
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
            
            var targetPos = Transform.Position + CameraOffset;
            
            // mainCamera.Entity.Transform.Position = Math.Lerp(mainCamera.Entity.Transform.Position, targetPos, CameraFollowSpeed * dt);
            mainCamera.Entity.Transform.Position = targetPos;
            
            float distance = (Transform.Position - mainCamera.Entity.Transform.Position).magnitudeF;
            
            mainCamera.Entity.Transform.Position = Transform.Position;
            
            mainCamera.Entity.Transform.RotateAround(Transform.Position, Vector3.Up, cameraHorizontal);
            mainCamera.Entity.Transform.RotateAround(Transform.Position, mainCamera.Entity.Transform.Right, cameraVertical);
            
            // var cameraRotation = mainCamera.Entity.Transform.Rotation;
            // cameraRotation.x -= cameraVertical;
            // cameraRotation.y += cameraHorizontal;
            
            // mainCamera.Entity.Transform.Rotation = cameraRotation;
            
            mainCamera.Entity.Transform.Position -= mainCamera.Entity.Transform.Forward * distance;
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
                    
                rigidbody.RotateTowards(Transform.Position + lastDirection * -1, RotationSpeed * fixedDeltaTime);
            }
            
            rigidbody.Velocity = velocity;
        }
    }
}
