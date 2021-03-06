using System;
using System.Collections.Generic;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;

namespace Waldem.SceneManagement.SceneManager
{
    public interface ISceneManager
    {
        Dictionary<Type, IScene> Scenes { get; set; }
        IScene CurrentScene { get; set; }

        void ChangeCurrentScene(IScene _scene);

        void ChangeCurrentScene<T>();

        void RunFirstScene();

        void NextScene();
        
        void PreviousScene();
        
        void RemoveScene<T>() where T:IScene;

        void AddScene(IScene _scene);

        IScene GetScene<T>() where T:IScene;

        void Draw(GameTime _gameTime);

        void Update(GameTime _gameTime);

        void Fade(FadeTypes type, float duration = 1, Action onEnd = null);

        enum FadeTypes{
            In, Out
        };
    }
}