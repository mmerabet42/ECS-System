/*

Engines controls the behaviours of Objects in a Scene.
Only Objects that passes the Engine's filters are behaved by the Engine.

*/

#pragma once

#include <iostream>
#include <tuple>
#include <vector>

#include "Object.hpp"
#include "EngineFilter.hpp"
#include "utils.hpp"
#include "Notifiable.hpp"

namespace dn
{
    // Forward declaration of the Scene class (Because the Scene class includes this file, this is to avoid include loops)
    class Scene;

    // An engine is defined by a list of filters.
    // The engine's behaviours are organised by filters, objects must at least pass one filter in order to be controlled by the engine.
    template <typename ... T_Filters>
    class EngineHelper;

    // This is the top of the engine helpers class, it takes 0 templates argument.
    // The engine helper classes are not meant to be used outside of this file, they are here to make all the system work.

    // An engine is a notifiable wich means that it has a notifier, wich might be notified by other notifiers,
    // if there are any changes in the components side or the objects side, the engine must be aware of that.
    template <>
    class EngineHelper<> : public dn::Notifiable<dn::Object *>
    {
    public:
        EngineHelper()
        {
            // If the engine notifier is notified somewhere, this callback is called.
            this->notifier().onNotification([this](dn::Object *p_object) {
                // The only thing it does is update the engine's state of the object,
                // if a component was detached to the object, it may no longer pass some filters,
                // or if a component was attached to the object, then it may pass some filters.
                this->updateObject(p_object);
            });
        }

        // These are overridable functions, that are called by the Scene.

        // This function is called once the Scene has started, or if the engine is added to a scene that has already started.
        virtual void onStart() {}
        // This function is called every time the scene updates
        virtual void onUpdate() {}
        // This function is called once the engine is destroyed (removed to the scene, or if the scene is destroyed)
        virtual void onDestroy() {}

        // Helper function overrided by the Engine class. This class is not aware of the engine's filters,
        // so we must call a function defined below in the class hierarchy.
        virtual void updateObjectHelper(dn::Object *p_object, bool p_remove) = 0;
        void updateObject(dn::Object *p_object, bool p_remove = false)
        {
            this->updateObjectHelper(p_object, p_remove);
        }

        // Same as above.
        virtual void cleanTrashHelper() = 0;
        // This function is called by the scene at the end of its update function, it cleans all the filters
        // that are in the trash.
        void cleanTrash()
        {
            this->cleanTrashHelper();
        }

        // Returns the scene to wich the engine has been added to.
        dn::Scene *scene() const
        {
            return this->_scene;
        }

    protected:
        dn::Scene *_scene;

        // The scene class must have access to the private attribute above.
        friend class dn::Scene;
    };

    // There we are using a certain power of the C++ language: recursive inheritance.
    // We needed a callback function for each engine's filters.
    // When an object is added to the scene, the engine tests the object on all its filters, if it passes one,
    // an instance of that filter is generated for this object. This filter is the object's fingerprint for this engine.
    // Once the filter is created and stored in the engine, the according callback is called. The C++ language,
    // knows which one to call thanks to the parameter it receives, function overloading allows that.
    template <typename T_Filter, typename ... T_Others>
    class EngineHelper<T_Filter, T_Others...> : public dn::EngineHelper<T_Others...>
    {
    public:
        // This function is called when an object passes the filter and before it is added to the engine,
        // if the function returns true, then it is added, otherwise if it returns false, it is cancelled.
        bool onObjectComingHelper(T_Filter &p_filter) { return this->onObjectComing(p_filter); }
        virtual bool onObjectComing(T_Filter &p_filter) { return true; }

        // This function is called when an object has passed the filter and added to the engine.
        void onObjectAddedHelper(T_Filter &p_filter) { this->onObjectAdded(p_filter); }
        virtual void onObjectAdded(T_Filter &p_filter) {}

        // This function is called when an object was removed to the engine.
        void onObjectRemovedHelper(T_Filter &p_filter) { this->onObjectRemoved(p_filter); }
        virtual void onObjectRemoved(T_Filter &p_filter) {}
    };

    // This is the class that must be inherited in order to create custom engines
    template <typename ... T_Filters>
    class Engine : public EngineHelper<T_Filters...>
    {
    public:
        ~Engine()
        {
            this->onDestroy();

            // The deleteFilters function is called for each filters
            ((this->deleteFilters<T_Filters>()), ...);
        }

        // Returns all the T_Filter filters.
        template <typename T_Filter>
        std::vector<T_Filter *> &getEntities()
        {
            return std::get<std::vector<T_Filter *>>(this->_filters);
        }

    private:

        // Called by the class destructor in order to cleanup everything.
        template <typename T_Filter>
        void deleteFilters()
        {
            for (auto &&filter : std::get<std::vector<T_Filter *>>(this->_filters))
                delete filter;
        }

        // The updateObject helper function is defined here.
        // The p_remove functions tells if the object must be removed even thought it passes some filters
        void updateObjectHelper(dn::Object *p_object, bool p_remove)
        {
            this->testFilter<T_Filters...>(p_object, p_remove);
        }

        // The testFilter will test the object on each filters.
        template <typename T_Filter, typename ... T_Others>
        void testFilter(dn::Object *p_object, bool p_remove)
        {
            std::vector<T_Filter *> &filters = std::get<std::vector<T_Filter *>>(this->_filters);

            auto &&it = std::find_if(filters.begin(), filters.end(), [&p_object](T_Filter *pf) {
                if (pf->object() == p_object && pf->active())
                    return true;
                return false;
            });

            if (!p_remove && T_Filter::passFilter(p_object))
            {
                if (it == filters.end())
                {
                    T_Filter *filter = T_Filter::template makeFilter<T_Filter>(p_object);

                    if (!dn::EngineHelper<T_Filter, T_Others...>::onObjectComingHelper(*filter))
                    {
                        delete filter;
                        return;
                    }
                    filters.push_back(filter);
                    dn::EngineHelper<T_Filter, T_Others...>::onObjectAddedHelper(*filter);
                }
            }
            else if (it != filters.end())
            {
                (*it)->setActive(false);
                std::get<TrashType<T_Filter>>(this->_trash).push_back(it);
                dn::EngineHelper<T_Filter, T_Others...>::onObjectRemovedHelper(**it);
            }

            if constexpr (sizeof...(T_Others) > 0)
                this->testFilter<T_Others...>(p_object, p_remove);
        }

        // The cleanTrash helper is defined here.
        void cleanTrashHelper()
        {
            this->cleanTrashOne<T_Filters...>();
        }

        // Same as the testFilter function, this function is called for each filters
        template <typename T_Filter, typename ... T_Others>
        void cleanTrashOne()
        {
            for (auto &it : std::get<TrashType<T_Filter>>(this->_trash))
            {
                delete *it;
                std::get<std::vector<T_Filter *>>(this->_filters).erase(it);
            }
            std::get<TrashType<T_Filter>>(this->_trash).clear();

            if constexpr (sizeof...(T_Others) > 0)
                this->cleanTrashOne<T_Others...>();
        }

    private:
        // The fitlers are stored in a tuple of filter's arrays, so there is a different array for each filters.
        std::tuple<std::vector<T_Filters *>...> _filters;
        
        // Same for the trash filter's array.
        template <typename T_Filter>
        using TrashType = std::vector<typename std::vector<T_Filter *>::iterator>;
        std::tuple<TrashType<T_Filters>...> _trash;
    };
}