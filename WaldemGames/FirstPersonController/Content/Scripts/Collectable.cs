using Waldem;

namespace Waldem
{
    public class Collectable : ScriptableEntity
    {
        public string DebugText = "You collected something!";

        protected override void OnTriggerEnter(Entity other)
        {
            Debug.Log(DebugText);
            Destroy();
        }
    }
}