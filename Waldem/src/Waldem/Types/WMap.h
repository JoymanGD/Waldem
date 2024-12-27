#pragma once
#include "WPair.h"

namespace Waldem
{
    template<typename T1, typename T2>
    struct WALDEM_API WMap
    {
    private:
        WArray<WPair<T1, T2>> Data;

    public:
        WMap() = default;

        void Insert(const T1& key, const T2& value)
        {
            Data.PushBack(WPair<T1, T2>(key, value));
        }

        void Add(const T1& key, const T2& value)
        {
            Data.Add(WPair<T1, T2>(key, value));
        }

        bool Remove(const T1& key)
        {
            for (size_t i = 0; i < Data.Num(); ++i)
            {
                if (Data[i].key == key)
                {
                    Data.Erase(i);
                    return true;
                }
            }
            return false;
        }

        bool Contains(const T1& key) const
        {
            for (size_t i = 0; i < Data.Num(); ++i)
            {
                if (Data[i].key == key)
                {
                    return true;
                }
            }
            return false;
        }

        T2* Find(const T1& key)
        {
            for (size_t i = 0; i < Data.Num(); ++i)
            {
                if (Data[i].key == key)
                {
                    return &Data[i].value;
                }
            }
            return nullptr;
        }

        T2& operator[](const T1& key)
        {
            for (size_t i = 0; i < Data.Num(); ++i)
            {
                if (Data[i].key == key)
                {
                    return Data[i].value;
                }
            }

            WD_CORE_ERROR("Key '{0}' not found in WMap", key);
            throw std::runtime_error("Key not found in WMap");
        }

        auto begin() { return Data.begin(); }
        auto end() { return Data.end(); }
    };
}
