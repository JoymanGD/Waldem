using Microsoft.Xna.Framework;
using MonoGame.Extended;
using MonoGame.Extended.Input;

namespace Waldem.UI.Elements
{
    public class Button : RectangleElement
    {
        public Label Label { get; private set; }
        public bool Clicked { get; private set; }
        public bool Focused { get; private set; }
        
        public Button(Vector2 position, Size2 size, string text) : base(position, size)
        {
            var labelPos = Position + (Vector2)Size/2;
            Label = new Label(labelPos, text, false);
        }
        
        public Button(Vector2 position, Size2 size, string text, Color textColor, Color backgroundColor, Color outlineColor) : base(position, size, backgroundColor, outlineColor)
        {
            var labelPos = Position + (Vector2)Size/2;
            Label = new Label(labelPos, text, textColor, false);
        }

        public override void Draw(GameTime gameTime)
        {
            base.Draw(gameTime);
            Label.Draw(gameTime);
        }

        public override void Update(GameTime gameTime){
            var mouseState = MouseExtended.GetState();
            
            Focused = Bounds.Contains(mouseState.Position);

            Clicked = Focused && mouseState.WasButtonJustDown(MouseButton.Left);
        }
    }
}