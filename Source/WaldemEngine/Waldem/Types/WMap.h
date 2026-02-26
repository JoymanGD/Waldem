#pragma once
#include "WPair.h"
#include "WArray.h"

namespace Waldem
{
    template<typename T1, typename T2>
    struct WMap
    {
    private:
        WArray<WPair<T1, T2>> Data;

    public:
        WMap() = default;

        WMap(std::initializer_list<WPair<T1, T2>> initList)
        {
            for (const auto& pair : initList)
            {
                Data.PushBack(pair);
            }
        }

        void Insert(const T1& key, const T2& value)
        {
            Data.PushBack(WPair<T1, T2>(key, value));
        }

        void Clear()
        {
            Data.Clear();
        }

        void Add(const T1& key, const T2& value)
        {
            Data.Add(WPair<T1, T2>(key, value));
        }

        void EmplaceBack(const T1& key, const T2& value)
        {
            Data.EmplaceBack(WPair<T1, T2>(key, value));
        }

        bool Remove(const T1& key)
        {
            for (size_t i = 0; i < Data.Num(); ++i)
            {
                if (Data[i].key == key)
                {
                    Data.RemoveAt(i);
                    return true;
                }
            }
            return false;
        }

        bool Remove(const WPair<T1, T2>& pair)
        {
            for (size_t i = 0; i < Data.Num(); ++i)
            {
                if (Data[i] == pair)
                {
                    Data.RemoveAt(i);
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

        WPair<T1, T2>* Find(const T1& key, const T2& value)
        {
            for (size_t i = 0; i < Data.Num(); ++i)
            {
                if (Data[i].key == key && Data[i].value == value)
                {
                    return &Data[i];
                }
            }
            return nullptr;
        }

        T2* Find(const T1& key)
        {
            for (size_t i = 0; i < Data.Num(); ++i)
            {
                if (Data[i].key == key)
                {
                    if constexpr (std::is_pointer_v<T2>)
                    {
                        return Data[i].value;
                    }
                    else
                    {
                        return &Data[i].value;
                    }
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
            
            WD_CORE_ERROR("Key not found in WMap");

            // If key not found, insert default-constructed value
            Data.Add(WPair<T1, T2>(key, T2{}));
            return Data.Last().value;
        }

        const T2& operator[](const T1& key) const
        {
            for (size_t i = 0; i < Data.Num(); ++i)
            {
                if (Data[i].key == key)
                {
                    return Data[i].value;
                }
            }
            
            throw std::out_of_range("Key not found in const WMap");
        }

        WPair<T1, T2>& operator[](int index)
        {
            return Data[index];
        }

        WPair<T1, T2>& At(int index)
        {
            return Data[index];
        }

        auto begin() { return Data.begin(); }
        auto end() { return Data.end(); }

        uint Num() const
        {
            return Data.Num();
        }

        bool IsEmpty() const
        {
            return Num() == 0;
        }
    };
}
