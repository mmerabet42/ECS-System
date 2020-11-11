/*

    A Scene controls everything, it starts the engines, updates, and notifies them.

*/

#pragma once

#include <map>
#include <vector>

#include "Object.hpp"
#include "Engine.hpp"
#include "Notifiable.hpp"

namespace dn
{
    // A scene is a notifable, it notifies the engines when a change is noticed in objects or components.
    class Scene : public dn::Notifiable<dn::Object *>
    {
    public:
        Scene()
            : _started(false)
        {
            this->_trashNotifier.onNotification([this](dn::Object *p_object, const bool &p_state) {
                if (p_state)
                    this->_objectsNeedClean.push_back(p_object);
                else
                {
                    auto &&it = std::find(this->_objectsNeedClean.begin(), this->_objectsNeedClean.end(), p_object);
                    if (it != this->_objectsNeedClean.end())
                        this->_objectsNeedClean.erase(it);
                }
            });
        }

        ~Scene()
        {
            for (auto &&engine : this->_engines)
                delete engine.second;
        }

    public:

        // Starts all the engines.
        void start()
        {
            if (this->_started)
                return;

            for (auto &&engine : this->_engines)
                engine.second->onStart();
            this->_started = true;
        }

        // Updates all the engines, and cleans if there is anything to clean.
        void update()
        {
            if (!this->_started)
                return;

            for (auto &&engine : this->_engines)
            {
                engine.second->onUpdate();
                engine.second->cleanTrash();
            }

            for (auto &&object : this->_objectsNeedClean)
                object->cleanTrash();
            this->_objectsNeedClean.clear();
        }

        // Adds an object to the scene, and sends it the engines.
        void addObject(dn::Object *p_object)
        {
            if (std::find(this->_objects.begin(), this->_objects.end(), p_object) != this->_objects.end())
                return;
            this->_objects.push_back(p_object);
            p_object->notifier().connect(this->notifier());

            p_object->trashNotifier().connect(this->_trashNotifier);

            for (auto &&engine : this->_engines)
                engine.second->updateObject(p_object);
        }

        // Remove an object to the scene.
        void removeObject(dn::Object *p_object)
        {
            auto &&it = std::find(this->_objects.begin(), this->_objects.end(), p_object);
            
            if (it == this->_objects.end())
                return;
            for (auto &&engine : this->_engines)
                engine.second->updateObject(p_object, true);
            this->_objects.erase(it);
        }

        // Adds an engine to the scene.
        template <typename T_Engine, typename ... T_Args>
        void addEngine(T_Args && ... p_args)
        {
            if (this->_engines.find(dn::getType<T_Engine>()) != this->_engines.end())
                return;
            
            T_Engine *engine = new T_Engine(std::forward<T_Args>(p_args)...);
            engine->_scene = this;
            this->notifier().connect(engine->notifier());

            for (auto &&obj : this->_objects)
                engine->updateObject(obj);

            this->_engines.emplace(dn::getType<T_Engine>(), (dn::EngineHelper<> *)engine);

            if (this->_started)
                engine->onStart();
        }

        template <typename T_Engine>
        T_Engine *getEngine()
        {
            auto &&it = this->_engines.find(dn::getType<T_Engine>());

            if (it == this->_engines.end())
                return nullptr;
            return dynamic_cast<T_Engine *>(it.second);
        }

        // Removes an engine from the scene.
        template <typename T_Engine>
        void removeEngine()
        {
            auto &&it = this->_engines.find(dn::getType<T_Engine>());

            if (it == this->_engines.end())
                return;
            delete *it;
            this->_engines.erase(it);
        }

    private:
        std::map<const std::type_info *, dn::EngineHelper<> *> _engines;

        std::vector<dn::Object *> _objects;
        std::vector<dn::Object *> _objectsNeedClean;
        dn::Notifier<dn::Object *, const bool &> _trashNotifier;

        bool _started;
    };
}