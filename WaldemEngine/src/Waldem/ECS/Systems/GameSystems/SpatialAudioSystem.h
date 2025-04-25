#pragma once
#include "Waldem/Audio/Audio.h"
#include "Waldem/ECS/Components/AudioListener.h"
#include "Waldem/ECS/Components/AudioSource.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/Transform.h"

namespace Waldem
{
    class WALDEM_API SpatialAudioSystem : public ISystem
    {
    private:
        float PAN_SIMPLIFICATION = 0.8f;        
    public:
        SpatialAudioSystem(ECSManager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
        }

        void Update(float deltaTime) override
        {
            Audio::LockAudioThread();
            
            for (auto [listenerEntity, audioListener, listenerTransform] : Manager->EntitiesWith<AudioListener, Transform>())
            {
                for (auto [sourceEntity, audioSource, sourceTransform] : Manager->EntitiesWith<AudioSource, Transform>())
                {
                    if(!audioSource.Spatial)
                    {
                        continue;
                    }
                    
                    if(!audioSource.Clip)
                    {
                        continue;
                    }

                    Vector3 listenerPos = listenerTransform.Position;
                    Vector3 sourcePos = sourceTransform.Position;
                    
                    //distance handling
                    float dx = sourcePos.x - listenerPos.x;
                    float dy = sourcePos.y - listenerPos.y;
                    float dz = sourcePos.z - listenerPos.z;
                    
                    float distance = sqrtf(dx*dx + dy*dy + dz*dz);
                    
                    float minDist = 1.0f;
                    float maxDist = audioSource.Range;
                    float rolloff = 1.0f;
                    
                    if (distance < minDist) distance = minDist;
                    
                    if (distance > maxDist)
                    {
                        audioSource.Clip->CurrentChannel->distanceVolume = 0.0f;
                    }
                    else
                    {
                        audioSource.Clip->CurrentChannel->distanceVolume = 1.0f / (1.0f + rolloff * (distance - minDist));
                    }

                    //simple distance handling
                    // float distance = glm::distance(listenerPos, sourcePos);
                    //
                    // if (distance > audioSource.Range)
                    // {
                    //     audioSource.Clip->CurrentChannel->distanceVolume = 0.0f;
                    // }
                    // else
                    // {
                    //     audioSource.Clip->CurrentChannel->distanceVolume = 1.0f - (distance / audioSource.Range);
                    // }
                    
                    //panning
                    Vector3 direction = normalize(sourcePos - listenerPos);
                    Vector3 right = normalize(listenerTransform.GetRightVector());
                    Vector3 forward = normalize(listenerTransform.GetForwardVector());
                    
                    float x = dot(direction, right);
                    float z = dot(direction, forward);
                    
                    float angle = atan2(x, z);
                    
                    float pan = glm::clamp(sin(angle), -1.0f, 1.0f);
                    audioSource.Clip->CurrentChannel->pan = pan * PAN_SIMPLIFICATION;
                }
            }

            Audio::UnlockAudioThread();
        }
    };
}
