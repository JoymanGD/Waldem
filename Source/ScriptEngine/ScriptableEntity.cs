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
        
        private void OnTriggerEnterInternal(ulong otherEntityId)
        {
            Entity otherEntity = new Entity();
            otherEntity.__SetEntityId(otherEntityId);
            
            OnTriggerEnter(otherEntity);
        }
 
        private void OnTriggerStayInternal(ulong otherEntityId)
        {
            Entity otherEntity = new Entity();
            otherEntity.__SetEntityId(otherEntityId);
            
            OnTriggerStay(otherEntity);
        }
 
        private void OnTriggerExitInternal(ulong otherEntityId)
        {
            Entity otherEntity = new Entity();
            otherEntity.__SetEntityId(otherEntityId);
            
            OnTriggerExit(otherEntity);
        }
 
        protected virtual void OnTriggerEnter(Entity other)
        {
        }
 
        protected virtual void OnTriggerStay(Entity other)
        {
        }
 
        protected virtual void OnTriggerExit(Entity other)
        {
        }
    }
}
