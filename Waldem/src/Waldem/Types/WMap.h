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
            throw std::out_of_range("Key not found");
        }

        auto begin() { return Data.begin(); }
        auto end() { return Data.end(); }
    };
}
