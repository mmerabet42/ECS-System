/*

    A component stores data for an object

*/

#pragma once

#include "Notifiable.hpp"

namespace dn
{
    // A component is a notifiable, it notifies the object it is attached to, if its active state changes.
    struct Component : public dn::Notifiable<>
    {
        Component()
            : _active(true)
        {}
        ~Component()
        {}

        // Returns if the component is active or not.
        bool active() const
        {
            return this->_active;
        }

        // Changes the current state of the component.
        void setActive(bool p_active)
        {
            this->_active = p_active;
            // Notifies the object notifier.
            this->notifier().notify();
        }

        // This function is called when the component is detached to the object
        virtual void onDestroy()
        {}

    private:
        bool _active;
    };
}