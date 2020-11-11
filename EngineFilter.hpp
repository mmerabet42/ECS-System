/*

    An EngineFilter tells wich components an object must have in order to pass it,
    and be added to an engine.

*/

#pragma once

#include <tuple>

#include "Object.hpp"

namespace dn
{
    // An engine filter is defined by a list of component types that must have an object.
    template <typename ... T_Components>
    class EngineFilter
    {
    public:
        EngineFilter()
            : _object(nullptr), _active(true)
        {}
        virtual ~EngineFilter() {}

        // Returns the object to wich the filter has been generated for.
        dn::Object *object() const
        {
            return this->_object;
        }

        // Returns if the filter is active or not.
        bool active() const
        {
            return this->_active;
        }

        // Changes the filter's active state.
        void setActive(const bool &p_active)
        {
            this->_active = p_active;
        }

        // Returns the component instance.
        template <typename T_Component>
        T_Component *get()
        {
            return std::get<T_Component *>(this->_components);
        }

        // It is a static function.
        // Generates a filter instance of that type for the given object.
        template <typename T_Filter>
        static T_Filter *makeFilter(dn::Object *p_object)
        {
            T_Filter *filter = new T_Filter;
            filter->_object = p_object;
            ((std::get<T_Components *>(filter->_components) = p_object->getComponent<T_Components>()), ...);
            return filter;
        }

        // Tests if the given object passes the filter.
        static bool passFilter(dn::Object *p_object)
        {
            return (passFilterComponent(p_object->getComponent<T_Components>()) && ...);
        }

        static bool passFilterComponent(dn::Component *p_component)
        {
            return p_component != nullptr && p_component->active();
        }

    protected:
        // The object to wich the filter has been generated for.
        dn::Object *_object;
        // Each component instance got from the object is stored in this tuple.
        std::tuple<T_Components *...> _components;

        bool _active;
    };
}