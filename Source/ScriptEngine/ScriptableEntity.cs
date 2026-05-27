namespace Waldem
{
    public abstract class ScriptableEntity : Entity
    {
        protected virtual void OnCreate()
        {
        }

        protected virtual void OnUpdate(float deltaTime)
        {
        }

        protected virtual void OnFixedUpdate(float fixedDeltaTime)
        {
        }

        protected virtual void OnDestroy()
        {
        }
 
        private void OnCollisionEnterInternal(ContactPoint contact)
        {
            Entity otherEntity = new Entity();
            otherEntity.__SetEntityId(contact.OtherEntityId);
            
            Collision collision = new Collision();
            collision.Contact = contact;
            collision.Other = otherEntity;
            OnCollisionEnter(collision);
        }
 
        private void OnCollisionStayInternal(ContactPoint contact)
        {
            Entity otherEntity = new Entity();
            otherEntity.__SetEntityId(contact.OtherEntityId);
            
            Collision collision = new Collision();
            collision.Contact = contact;
            collision.Other = otherEntity;
            OnCollisionStay(collision);
        }
 
        private void OnCollisionExitInternal(ContactPoint contact)
        {
            Entity otherEntity = new Entity();
            otherEntity.__SetEntityId(contact.OtherEntityId);
            
            Collision collision = new Collision();
            collision.Contact = contact;
            collision.Other = otherEntity;
            OnCollisionExit(collision);
        }
 
        protected virtual void OnCollisionEnter(Collision collision)
        {
        }
 
        protected virtual void OnCollisionStay(Collision collision)
        {
        }
 
        protected virtual void OnCollisionExit(Collision collision)
        {
        }
    }
}
