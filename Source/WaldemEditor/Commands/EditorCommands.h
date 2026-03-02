#pragma once

#include <memory>
#include <vector>
#include <cstring>
#include <string>

#include "Waldem/ECS/ECS.h"
#include "Waldem/Utils/ECSUtils.h"

namespace Waldem
{
    struct ComponentValueBlob
    {
        ECS::IdT ComponentId = 0;
        void* Data = nullptr;

        ComponentValueBlob() = default;

        ComponentValueBlob(ECS::IdT componentId, const void* src)
        {
            Capture(componentId, src);
        }

        ComponentValueBlob(const ComponentValueBlob& other)
        {
            if(other.IsValid())
            {
                Capture(other.ComponentId, other.Data);
            }
        }

        ComponentValueBlob(ComponentValueBlob&& other) noexcept
        {
            ComponentId = other.ComponentId;
            Data = other.Data;

            other.ComponentId = 0;
            other.Data = nullptr;
        }

        ComponentValueBlob& operator=(const ComponentValueBlob& other)
        {
            if(this == &other)
            {
                return *this;
            }

            Clear();
            if(other.IsValid())
            {
                Capture(other.ComponentId, other.Data);
            }

            return *this;
        }

        ComponentValueBlob& operator=(ComponentValueBlob&& other) noexcept
        {
            if(this == &other)
            {
                return *this;
            }

            Clear();

            ComponentId = other.ComponentId;
            Data = other.Data;

            other.ComponentId = 0;
            other.Data = nullptr;

            return *this;
        }

        ~ComponentValueBlob()
        {
            Clear();
        }

        bool IsValid() const
        {
            return ComponentId != 0 && Data != nullptr;
        }

        void Capture(ECS::IdT componentId, const void* src)
        {
            Clear();

            if(componentId == 0 || src == nullptr)
            {
                return;
            }

            const ecs_type_info_t* typeInfo = ECS::GetTypeInfo(componentId);
            if(typeInfo == nullptr || typeInfo->size == 0)
            {
                return;
            }

            ComponentId = componentId;
            Data = ECS::ValueNew(ComponentId);
            if(Data == nullptr)
            {
                ComponentId = 0;
                return;
            }

            ECS::ValueCopy(ComponentId, Data, src);
        }

        void ApplyTo(void* dst) const
        {
            if(!IsValid() || dst == nullptr)
            {
                return;
            }

            ECS::ValueCopy(ComponentId, dst, Data);
        }

        bool Equals(const ComponentValueBlob& other) const
        {
            if(!IsValid() || !other.IsValid())
            {
                return false;
            }

            if(ComponentId != other.ComponentId)
            {
                return false;
            }

            const ecs_type_info_t* typeInfo = ECS::GetTypeInfo(ComponentId);
            if(typeInfo == nullptr)
            {
                return false;
            }

            const bool equalsIsLegal = (typeInfo->hooks.flags & ECS_TYPE_HOOK_EQUALS_ILLEGAL) == 0;
            if(typeInfo->hooks.equals && equalsIsLegal)
            {
                return typeInfo->hooks.equals(Data, other.Data, typeInfo);
            }

            return memcmp(Data, other.Data, (size_t)typeInfo->size) == 0;
        }

    private:
        void Clear()
        {
            if(IsValid())
            {
                ECS::ValueFree(ComponentId, Data);
            }

            ComponentId = 0;
            Data = nullptr;
        }
    };

    class IEditorCommand
    {
    public:
        virtual ~IEditorCommand() = default;
        virtual void Do() = 0;
        virtual void Undo() = 0;
        virtual bool TryMerge(const IEditorCommand& other) { return false; }
    };

    class EditorCommandHistory
    {
    public:
        static EditorCommandHistory& Get()
        {
            static EditorCommandHistory* history = new EditorCommandHistory();
            return *history;
        }

        bool IsReplaying() const { return Replaying; }

        void Execute(std::unique_ptr<IEditorCommand> command)
        {
            if(!command)
            {
                return;
            }

            command->Do();

            if(!UndoStack.empty() && UndoStack.back()->TryMerge(*command))
            {
                RedoStack.clear();
                return;
            }

            UndoStack.emplace_back(std::move(command));
            RedoStack.clear();
        }

        void Undo()
        {
            if(UndoStack.empty())
            {
                return;
            }

            Replaying = true;
            auto command = std::move(UndoStack.back());
            UndoStack.pop_back();
            command->Undo();
            RedoStack.emplace_back(std::move(command));
            Replaying = false;
        }

        void Redo()
        {
            if(RedoStack.empty())
            {
                return;
            }

            Replaying = true;
            auto command = std::move(RedoStack.back());
            RedoStack.pop_back();
            command->Do();
            UndoStack.emplace_back(std::move(command));
            Replaying = false;
        }

        void Clear()
        {
            UndoStack.clear();
            RedoStack.clear();
            Replaying = false;
        }

    private:
        bool Replaying = false;
        std::vector<std::unique_ptr<IEditorCommand>> UndoStack;
        std::vector<std::unique_ptr<IEditorCommand>> RedoStack;
    };

    class SetComponentDataCommand : public IEditorCommand
    {
    public:
        SetComponentDataCommand(ECS::EntityT entityId, ECS::IdT componentId, const ComponentValueBlob& before, const ComponentValueBlob& after, bool allowMerge = true)
            : EntityId(entityId), ComponentId(componentId), Before(before), After(after), AllowMerge(allowMerge)
        {
        }

        void Do() override
        {
            Apply(After);
        }

        void Undo() override
        {
            Apply(Before);
        }

        bool TryMerge(const IEditorCommand& other) override
        {
            if(!AllowMerge)
            {
                return false;
            }

            const auto* typed = dynamic_cast<const SetComponentDataCommand*>(&other);
            if(typed == nullptr)
            {
                return false;
            }

            if(!typed->AllowMerge || typed->EntityId != EntityId || typed->ComponentId != ComponentId)
            {
                return false;
            }

            After = typed->After;
            return true;
        }

    private:
        void Apply(const ComponentValueBlob& state)
        {
            if(!state.IsValid())
            {
                return;
            }

            auto entity = ECS::World.entity(EntityId);
            if(!entity.is_alive())
            {
                return;
            }

            ECS::Id component(ECS::World, ComponentId);

            if(!entity.has(component))
            {
                entity.add(component);
            }

            const ecs_type_info_t* typeInfo = ECS::GetTypeInfo(ComponentId);
            const bool hasComponentData = typeInfo != nullptr && typeInfo->size > 0;
            if (hasComponentData)
            {
                void* addedValue = entity.get_mut(component);
                if(addedValue == nullptr)
                {
                    return;
                }
                entity.modified(component);
            }

            void* dst = entity.get_mut(component);
            if(dst == nullptr)
            {
                return;
            }

            state.ApplyTo(dst);
            entity.modified(component);
        }

    private:
        ECS::EntityT EntityId = 0;
        ECS::IdT ComponentId = 0;
        ComponentValueBlob Before;
        ComponentValueBlob After;
        bool AllowMerge = true;
    };

    class RenameEntityCommand : public IEditorCommand
    {
    public:
        RenameEntityCommand(ECS::EntityT entityId, const WString& before, const WString& after)
            : EntityId(entityId), Before(before), After(after)
        {
        }

        void Do() override
        {
            auto entity = ECS::World.entity(EntityId);
            if(entity.is_alive())
            {
                entity.set_name(After.C_Str());
            }
        }

        void Undo() override
        {
            auto entity = ECS::World.entity(EntityId);
            if(entity.is_alive())
            {
                entity.set_name(Before.C_Str());
            }
        }

    private:
        ECS::EntityT EntityId = 0;
        WString Before;
        WString After;
    };

    class CreateSceneEntityCommand : public IEditorCommand
    {
    public:
        CreateSceneEntityCommand(const WString& name, bool enabled = true, bool visibleInHierarchy = true)
            : RequestedName(name), Enabled(enabled), VisibleInHierarchy(visibleInHierarchy)
        {
        }

        ECS::EntityT GetEntityId() const { return EntityId; }

        void Do() override
        {
            if(EntitySnapshot.empty())
            {
                auto entity = ECS::CreateSceneEntity(RequestedName, Enabled, VisibleInHierarchy);
                EntityId = entity.id();
                EntitySnapshot = entity.to_json().c_str();
                return;
            }

            auto entity = ECS::World.entity();
            entity.from_json(EntitySnapshot.c_str());
            EntityId = entity.id();
        }

        void Undo() override
        {
            auto entity = ECS::World.entity(EntityId);
            if(entity.is_alive())
            {
                entity.destruct();
            }
        }

    private:
        WString RequestedName;
        bool Enabled = true;
        bool VisibleInHierarchy = true;
        ECS::EntityT EntityId = 0;
        std::string EntitySnapshot;
    };

    class CloneSceneEntityCommand : public IEditorCommand
    {
    public:
        explicit CloneSceneEntityCommand(ECS::EntityT sourceId)
            : SourceId(sourceId)
        {
        }

        ECS::EntityT GetCloneId() const { return CloneId; }

        void Do() override
        {
            if(CloneSnapshot.empty())
            {
                auto source = ECS::World.entity(SourceId);
                if(!source.is_alive())
                {
                    return;
                }

                auto clone = ECS::CloneSceneEntity(source);
                CloneId = clone.id();
                CloneSnapshot = clone.to_json().c_str();
                return;
            }

            auto clone = ECS::World.entity();
            clone.from_json(CloneSnapshot.c_str());
            CloneId = clone.id();
        }

        void Undo() override
        {
            auto clone = ECS::World.entity(CloneId);
            if(clone.is_alive())
            {
                clone.destruct();
            }
        }

    private:
        ECS::EntityT SourceId = 0;
        ECS::EntityT CloneId = 0;
        std::string CloneSnapshot;
    };

    class DeleteSceneEntityCommand : public IEditorCommand
    {
    public:
        explicit DeleteSceneEntityCommand(ECS::Entity entity)
        {
            EntityId = entity.id();
            if(entity.is_alive())
            {
                EntitySnapshot = entity.to_json().c_str();
            }
        }

        void Do() override
        {
            auto entity = ECS::World.entity(EntityId);
            if(entity.is_alive())
            {
                entity.destruct();
            }
        }

        void Undo() override
        {
            if(EntitySnapshot.empty())
            {
                return;
            }

            auto entity = ECS::World.entity();
            entity.from_json(EntitySnapshot.c_str());
            EntityId = entity.id();
        }

    private:
        ECS::EntityT EntityId = 0;
        std::string EntitySnapshot;
    };

    class AddComponentCommand : public IEditorCommand
    {
    public:
        AddComponentCommand(ECS::EntityT entityId, ECS::IdT componentId)
            : EntityId(entityId), ComponentId(componentId)
        {
        }

        void Do() override
        {
            auto entity = ECS::World.entity(EntityId);
            ECS::Id component(ECS::World, ComponentId);

            if(!entity.is_alive() || entity.has(component))
            {
                return;
            }

            entity.add(component);

            const ecs_type_info_t* typeInfo = ECS::GetTypeInfo(ComponentId);
            const bool hasComponentData = typeInfo != nullptr && typeInfo->size > 0;
            if (!hasComponentData)
            {
                return;
            }

            void* addedValue = entity.get_mut(component);
            if(addedValue == nullptr)
            {
                return;
            }
            entity.modified(component);
        }

        void Undo() override
        {
            auto entity = ECS::World.entity(EntityId);
            ECS::Id component(ECS::World, ComponentId);
            if(!entity.is_alive() || !entity.has(component))
            {
                return;
            }

            entity.remove(component);
        }

    private:
        ECS::EntityT EntityId = 0;
        ECS::IdT ComponentId = 0;
    };

    class RemoveComponentCommand : public IEditorCommand
    {
    public:
        RemoveComponentCommand(ECS::Entity entity, ECS::IdT componentId)
            : EntityId(entity.id()), ComponentId(componentId)
        {
            ECS::Id component(ECS::World, ComponentId);

            if(entity.is_alive() && entity.has(component))
            {
                const void* src = entity.get(component);
                if(src)
                {
                    RemovedValue.Capture(ComponentId, src);
                }
            }
        }

        void Do() override
        {
            auto entity = ECS::World.entity(EntityId);
            ECS::Id component(ECS::World, ComponentId);
            if(!entity.is_alive() || !entity.has(component))
            {
                return;
            }

            entity.remove(component);
        }

        void Undo() override
        {
            auto entity = ECS::World.entity(EntityId);
            if(!entity.is_alive() || !RemovedValue.IsValid())
            {
                return;
            }

            ECS::Id component(ECS::World, ComponentId);
            if(!entity.has(component))
            {
                entity.add(component);
            }

            void* dst = entity.get_mut(component);
            if(dst == nullptr)
            {
                return;
            }

            RemovedValue.ApplyTo(dst);
            entity.modified(component);
        }

    private:
        ECS::EntityT EntityId = 0;
        ECS::IdT ComponentId = 0;
        ComponentValueBlob RemovedValue;
    };

    class SetParentCommand : public IEditorCommand
    {
    public:
        SetParentCommand(ECS::EntityT childId, ECS::EntityT newParentId, bool keepWorldTransform = true)
            : ChildId(childId), ParentAfterId(newParentId), KeepWorldTransform(keepWorldTransform)
        {
            auto child = ECS::World.entity(ChildId);
            if(child.is_alive())
            {
                auto currentParent = ECS::GetParent(child);
                ParentBeforeId = currentParent.is_alive() ? currentParent.id() : 0;
            }
        }

        void Do() override
        {
            ApplyParent(ParentAfterId);
        }

        void Undo() override
        {
            ApplyParent(ParentBeforeId);
        }

    private:
        void ApplyParent(ECS::EntityT parentId)
        {
            auto child = ECS::World.entity(ChildId);
            if(!child.is_alive())
            {
                return;
            }

            if(parentId == 0)
            {
                ECS::ClearParent(child, KeepWorldTransform);
                return;
            }

            auto parent = ECS::World.entity(parentId);
            if(parent.is_alive())
            {
                ECS::SetParent(child, parent, KeepWorldTransform);
            }
            else
            {
                ECS::ClearParent(child, KeepWorldTransform);
            }
        }

    private:
        ECS::EntityT ChildId = 0;
        ECS::EntityT ParentBeforeId = 0;
        ECS::EntityT ParentAfterId = 0;
        bool KeepWorldTransform = true;
    };

    class SetParentsBatchCommand : public IEditorCommand
    {
    public:
        SetParentsBatchCommand(const std::vector<ECS::EntityT>& childIds, ECS::EntityT newParentId, bool keepWorldTransform = true)
            : ChildIds(childIds), ParentAfterId(newParentId), KeepWorldTransform(keepWorldTransform)
        {
            ParentBeforeIds.reserve(ChildIds.size());
            for (auto childId : ChildIds)
            {
                ECS::EntityT parentBeforeId = 0;
                auto child = ECS::World.entity(childId);
                if(child.is_alive())
                {
                    auto currentParent = ECS::GetParent(child);
                    if(currentParent.is_alive())
                    {
                        parentBeforeId = currentParent.id();
                    }
                }

                ParentBeforeIds.push_back(parentBeforeId);
            }
        }

        void Do() override
        {
            for (auto childId : ChildIds)
            {
                ApplyParent(childId, ParentAfterId);
            }
        }

        void Undo() override
        {
            const size_t count = std::min(ChildIds.size(), ParentBeforeIds.size());
            for (size_t i = 0; i < count; ++i)
            {
                ApplyParent(ChildIds[i], ParentBeforeIds[i]);
            }
        }

    private:
        void ApplyParent(ECS::EntityT childId, ECS::EntityT parentId)
        {
            auto child = ECS::World.entity(childId);
            if(!child.is_alive())
            {
                return;
            }

            if(parentId == 0)
            {
                ECS::ClearParent(child, KeepWorldTransform);
                return;
            }

            auto parent = ECS::World.entity(parentId);
            if(parent.is_alive())
            {
                ECS::SetParent(child, parent, KeepWorldTransform);
            }
            else
            {
                ECS::ClearParent(child, KeepWorldTransform);
            }
        }

    private:
        std::vector<ECS::EntityT> ChildIds;
        std::vector<ECS::EntityT> ParentBeforeIds;
        ECS::EntityT ParentAfterId = 0;
        bool KeepWorldTransform = true;
    };
}
