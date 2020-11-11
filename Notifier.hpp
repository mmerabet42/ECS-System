#pragma once

#include <functional>
#include <utility>
#include <vector>
#include <algorithm>

namespace dn
{
    template <typename ... T_Args>
    class Notifier
    {
    public:
        Notifier()
            : _notifyCallback(nullptr)
        {}
        ~Notifier()
        {
            for (auto &&notifier : this->_notifiedBy)
            {
                auto &&it = std::find(notifier->_notifiers.begin(), notifier->_notifiers.end(), this);

                if (it != notifier->_notifiers.end())
                    notifier->_notifiers.erase(it);
            }
        }

        void connect(dn::Notifier<T_Args...> &p_notifier)
        {
            this->_notifiers.push_back(&p_notifier);
            p_notifier._notifiedBy.push_back(this);
        }

        void disconnect(dn::Notifier<T_Args...> &p_notifier)
        {
            auto &&it = std::find(this->_notifiers.begin(), this->_notifiers.end(), &p_notifier);

            if (it != this->_notifiers.end())
            {
                this->_notifiers.erase(it);

                auto &&itBy = std::find(it->_notifiedBy.begin(), it->_notifiedBy.end(), this);
                if (itBy != it->_notifiedBy.end())
                    it->_notifiedBy.erase(itBy);
            }
        }

        void notify(T_Args && ... p_args)
        {
            if (this->_notifyCallback)
                this->_notifyCallback(std::forward<T_Args>(p_args)...);

            for (auto &&notifier: this->_notifiers)
                notifier->notify(std::forward<T_Args>(p_args)...);
        }

        void notifyLast(T_Args && ... p_args)
        {
            for (auto &&notifier: this->_notifiers)
                notifier->notifyLast(std::forward<T_Args>(p_args)...);

            if (this->_notifyCallback)
                this->_notifyCallback(std::forward<T_Args>(p_args)...);
        }

        void onNotification(const std::function<void(T_Args...)> &p_notifyCallback)
        {
            this->_notifyCallback = p_notifyCallback;
        }

    private:
        std::function<void(T_Args...)> _notifyCallback;

        std::vector<dn::Notifier<T_Args...> *> _notifiers;
        std::vector<dn::Notifier<T_Args...> *> _notifiedBy;
    };
}