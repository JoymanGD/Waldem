#pragma once
#include "Waldem/Audio/Audio.h"
#include "Waldem/ECS/Components/AudioListener.h"
#include "Waldem/ECS/Components/AudioSource.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/Transform.h"

namespace Waldem
{
    class WALDEM_API SpatialAudioSystem : public ICoreSystem
    {
    private:
        float PAN_SIMPLIFICATION = 0.8f;
    public:
        SpatialAudioSystem() {}
        
        void Initialize() override
        {
            ECS::World.observer<AudioSource>().event(flecs::OnSet).each([&](AudioSource& audioSource)
            {
                bool referenceIsEmpty = audioSource.ClipRef.Reference.empty() || audioSource.ClipRef.Reference == "Empty";
                    
                if(referenceIsEmpty && !audioSource.ClipRef.IsValid())
                {
                    return;
                }

                if(!referenceIsEmpty && !audioSource.ClipRef.IsValid())
                {
                    audioSource.ClipRef.LoadAsset();
                }
            });
            
            ECS::World.system<AudioListener, Transform>("Spatial audio system").kind(flecs::OnUpdate).each([&](AudioListener, Transform& listenerTransform)
            {
                auto audioSourcesQuery = ECS::World.query<AudioSource, Transform>();
                
                audioSourcesQuery.each([&](AudioSource& source, Transform& sourceTransform)
                {
                    if(!source.Spatial)
                    {
                        return;
                    }
                    
                    if(!source.ClipRef.IsValid() || !source.ClipRef.Clip->CurrentChannel)
                    {
                        return;
                    }

                    Vector3 listenerPos = listenerTransform.Position;
                    Vector3 sourcePos = sourceTransform.Position;
                    
                    //distance handling
                    float dx = sourcePos.x - listenerPos.x;
                    float dy = sourcePos.y - listenerPos.y;
                    float dz = sourcePos.z - listenerPos.z;
                    
                    float distance = sqrtf(dx*dx + dy*dy + dz*dz);
                    
                    float minDist = 1.0f;
                    float maxDist = source.Range;
                    float rolloff = 1.0f;
                    
                    if (distance < minDist) distance = minDist;
                    
                    if (distance > maxDist)
                    {
                        source.ClipRef.Clip->CurrentChannel->distanceVolume = 0.0f;
                    }
                    else
                    {
                        source.ClipRef.Clip->CurrentChannel->distanceVolume = 1.0f / (1.0f + rolloff * (distance - minDist));
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
                    source.ClipRef.Clip->CurrentChannel->pan = pan * PAN_SIMPLIFICATION;
                });
            });
        }

        void Deinitialize() override
        {
            ECS::World.query<AudioSource>().each([&](AudioSource& audioSource)
            {
                if (audioSource.ClipRef.IsValid() && audioSource.ClipRef.Clip->CurrentChannel)
                {
                    Audio::Stop(audioSource.ClipRef.Clip);
                }
            });
        }
    };
}
