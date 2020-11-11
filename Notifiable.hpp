#pragma once

#include "Notifier.hpp"

namespace dn
{
    // A notifiable is a class that can notify other notifier, or be notified.
    template <typename ... T_Args>
    class Notifiable
    {
    public:
        virtual ~Notifiable()
        {}

        dn::Notifier<T_Args ...> &notifier()
        {
            return this->_notifier;
        }

    private:
        dn::Notifier<T_Args ...> _notifier;
    };
}