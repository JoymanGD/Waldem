using Waldem;

namespace Waldem
{
    public class FirstPersonController : ScriptableEntity
    {
        public float MoveSpeed = 10.0f;
        public float PushForce = 20.0f;
        public float CameraOrbitSensitivity = 0.2f;
        public float CameraPitchMin = -70.0f;
        public float CameraPitchMax = 75.0f;

        private Vector3 movement;
        private Camera mainCamera;
        private CharacterController characterController;
        private float cameraPitch;

        protected override void OnCreate()
        {
            mainCamera = Camera.Main;
            characterController = GetComponent<CharacterController>();

            if (mainCamera != null)
            {
                cameraPitch = NormalizeAngle(mainCamera.Entity.Transform.Rotation.x);
                cameraPitch = Clamp(cameraPitch, CameraPitchMin, CameraPitchMax);
            }
        }

        protected override void OnUpdate(float deltaTime)
        {
            movement = Vector3.Zero;

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
                
                if (movement.magnitude > 0)
                    movement.Normalize();

                if (movement.magnitude > 0)
                {
                    if (characterController != null)
                    {
                        Vector3 controllerVelocity = characterController.MoveVelocity;
                        controllerVelocity.x = movement.x * MoveSpeed;
                        controllerVelocity.z = movement.z * MoveSpeed;
                        characterController.MoveVelocity = controllerVelocity;
                    }
                    else
                    {
                        Transform.Position += movement * MoveSpeed * deltaTime;
                    }
                }
                else if (characterController != null)
                {
                    Vector3 controllerVelocity = characterController.MoveVelocity;
                    controllerVelocity.x = 0;
                    controllerVelocity.z = 0;
                    characterController.MoveVelocity = controllerVelocity;
                }

                if (Input.GetKeyDown(KeyCode.Space))
                {
                    if (characterController != null)
                    {
                        characterController.Jump();
                    }
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

        protected override void OnLateUpdate(float deltaTime)
        {
            if (mainCamera != null)
            {
                UpdateFirstPersonCamera();
            }
        }

        protected override void OnCollisionStay(Collision collision)
        {
            if (collision.Other == null || !collision.Other.HasComponent<RigidBody>())
                return;

            RigidBody otherRigidBody = collision.Other.GetComponent<RigidBody>();
            if (otherRigidBody == null || otherRigidBody.IsKinematic)
                return;

            Vector3 pushDirection = Vector3.Zero;
            if (characterController != null)
            {
                pushDirection = characterController.MoveVelocity;
            }
            else
            {
                pushDirection = movement * MoveSpeed;
            }

            pushDirection.y = 0.0f;
            if (pushDirection.magnitude <= 0.0f)
                return;

            pushDirection.Normalize();
            otherRigidBody.ApplyImpulse(pushDirection * PushForce);
        }
    }
}
