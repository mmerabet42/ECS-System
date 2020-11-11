/*

    An Object is an abstract thing by itself, it is defined by its components, they stores different datas for him.

*/

#pragma once

#include <map>
#include <vector>
#include <typeinfo>
#include <algorithm>
#include <utility>

#include "Component.hpp"
#include "utils.hpp"
#include "Notifiable.hpp"

namespace dn
{
    // An object is a notifiable, it notifies the scene for any changes,
    // a component was added, removed, or if its active state has changed.
    class Object : public dn::Notifiable<dn::Object *>
    {
    public:
        ~Object()
        {
            this->onDestroy();

            // When an object is destroy every attached component is destroyed.
            for (auto &&component : this->_components)
                delete component.second;
        }

        // This function is called once the object is destroyed.
        virtual void onDestroy()
        {}

        // Attaches a component of type T_Component to the object.
        template <typename T_Component, typename ... T_Args>
        T_Component *addComponent(T_Args && ... p_args)
        {
            auto &&it = this->_components.find(dn::getType<T_Component>());

            if (it != this->_components.end() && it->second)
            {
                if (it->second->active())
                    return dynamic_cast<T_Component *>(it->second);
                it->second->setActive(true);

                auto &&itTrash = std::find(this->_trash.begin(), this->_trash.end(), it);
                if (itTrash == this->_trash.end())
                    return dynamic_cast<T_Component *>(it->second);
                this->_trash.erase(itTrash);
                this->_trashNotifier.notify(this, false);
                return dynamic_cast<T_Component *>(it->second);
            }

            T_Component *comp = new T_Component(std::forward<T_Args>(p_args)...);
            comp->notifier().onNotification([this]() {
                this->notifier().notify(this);
            });
            this->_components.emplace(dn::getType<T_Component>(), (dn::Component *)comp);
            this->notifier().notify(this);
            return comp;
        }

        // Returns the component of that type.
        template <typename T_Component>
        T_Component *getComponent()
        {
            auto &&it = this->_components.find(dn::getType<T_Component>());

            if (it != this->_components.end())
                return dynamic_cast<T_Component *>(it->second);
            return nullptr;
        }

        // Remove the component of that type.
        template <typename T_Component>
        bool removeComponent()
        {
            auto &&it = this->_components.find(dn::getType<T_Component>());

            if (it == this->_components.end())
                return false;
            this->_trash.push_back(it);
            this->_trashNotifier.notify(this, true);
            it->second->setActive(false);
            return true;
        }

        // Cleans the trash of components.
        void cleanTrash()
        {
            for (auto &&c : this->_trash)
            {
                delete c->second;
                this->_components.erase(c);
            }
            this->_trash.clear();
        }

        dn::Notifier<dn::Object *, const bool &> &trashNotifier()
        {
            return this->_trashNotifier;
        }

        std::string name;
    private:
        std::map<const std::type_info *, dn::Component *> _components;
        std::vector<std::map<const std::type_info *, dn::Component *>::iterator> _trash;

        dn::Notifier<dn::Object *, const bool &> _trashNotifier;
    };
}