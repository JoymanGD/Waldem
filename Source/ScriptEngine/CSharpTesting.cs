namespace Waldem
{
    public class CSharpTesting : ScriptableEntity
    {
        public float MoveSpeed = 3.0f;

        protected override void OnUpdate(float deltaTime)
        {
            Vector3 movement = Vector3.Zero;

            if (Input.IsKeyPressed(KeyCode.W))
                movement.z += 1.0f;
            if (Input.IsKeyPressed(KeyCode.S))
                movement.z -= 1.0f;
            if (Input.IsKeyPressed(KeyCode.A))
                movement.x -= 1.0f;
            if (Input.IsKeyPressed(KeyCode.D))
                movement.x += 1.0f;

            if (movement.x != 0.0f || movement.y != 0.0f || movement.z != 0.0f)
                Transform.Translate(movement * (MoveSpeed * deltaTime));
        }
    }
}
