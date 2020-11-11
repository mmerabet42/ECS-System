#pragma once

#include <typeinfo>

namespace dn
{
    template <typename T_Type>
    const std::type_info *getType()
    {
        static const std::type_info &ti = typeid(T_Type);
        return &ti;
    }

    template <typename T_Type>
    const std::type_info *getType(const T_Type &p_v)
    {
        return dn::getType<T_Type>();
    }
}