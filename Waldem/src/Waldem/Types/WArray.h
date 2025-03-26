#pragma once

namespace Waldem
{
    #include <vector>
    #include <stdexcept>
    #include <algorithm>
    #include <utility>

    template <typename T>
    class WArray
    {
    private:
        std::vector<T> ArrayInternal;

    public:
        WArray() = default; 

        explicit WArray(size_t size) : ArrayInternal(size) {} 

        WArray(size_t size, const T& defaultValue) : ArrayInternal(size, defaultValue) {} 

        WArray(std::initializer_list<T> initList) : ArrayInternal(initList) {}
        
        T& operator[](size_t index)
        {
            return ArrayInternal.at(index); 
        }

        const T& operator[](size_t index) const
        {
            return ArrayInternal.at(index); 
        }

        T& At(size_t index)
        {
            if (index >= ArrayInternal.size()) throw std::out_of_range("Index out of bounds");
            return ArrayInternal[index];
        }

        const T& At(size_t index) const
        {
            if (index >= ArrayInternal.size()) throw std::out_of_range("Index out of bounds");
            return ArrayInternal[index];
        }

        T* Find(const T& value)
        {
            auto it = std::find(ArrayInternal.begin(), ArrayInternal.end(), value);
            return it != ArrayInternal.end() ? &(*it) : nullptr;
        }
        
        size_t Num() const { return ArrayInternal.size(); }
        size_t Capacity() const { return ArrayInternal.capacity(); }
        bool IsEmpty() const { return ArrayInternal.empty(); }

        void Reserve(size_t capacity) { ArrayInternal.reserve(capacity); }
        void Resize(size_t newSize, const T& defaultValue = T()) { ArrayInternal.resize(newSize, defaultValue); }

        T* GetData() { return ArrayInternal.data(); }
        size_t GetSize() const { return ArrayInternal.size() * sizeof(T); }
        
        size_t Add(const T& value) { ArrayInternal.push_back(value); return ArrayInternal.size() - 1; }
        size_t Add(T&& value) { ArrayInternal.push_back(std::move(value)); return ArrayInternal.size() - 1; }

        void EmplaceBack(T& value) { ArrayInternal.emplace_back(value); }
        void EmplaceBack(T&& value) { ArrayInternal.emplace_back(std::move(value)); }

        void AddRange(const WArray<T>& other)
        {
            ArrayInternal.insert(ArrayInternal.end(), other.begin(), other.end());
        }
        
        void AddAt(const T& value, size_t index)
        {
            if (index > ArrayInternal.size()) throw std::out_of_range("Index out of bounds");
            ArrayInternal.insert(ArrayInternal.begin() + index, value);
        }

        void RemoveAt(size_t index)
        {
            if (index >= ArrayInternal.size()) throw std::out_of_range("Index out of bounds");
            ArrayInternal.erase(ArrayInternal.begin() + index);
        }

        void Remove(const T& value)
        {
            auto it = std::remove(ArrayInternal.begin(), ArrayInternal.end(), value);
            ArrayInternal.erase(it, ArrayInternal.end());
        }

        void RemoveLast()
        {
            if (ArrayInternal.empty()) throw std::out_of_range("Cannot pop from an empty array");
            ArrayInternal.pop_back();
        }

        void Clear() { ArrayInternal.clear(); }

        
        T& First()
        {
            if (ArrayInternal.empty()) throw std::out_of_range("Array is empty");
            return ArrayInternal.front();
        }

        T& Last()
        {
            if (ArrayInternal.empty()) throw std::out_of_range("Array is empty");
            return ArrayInternal.back();
        }

        const T& First() const
        {
            if (ArrayInternal.empty()) throw std::out_of_range("Array is empty");
            return ArrayInternal.front();
        }

        const T& Last() const
        {
            if (ArrayInternal.empty()) throw std::out_of_range("Array is empty");
            return ArrayInternal.back();
        }

        typename std::vector<T>::iterator begin() { return ArrayInternal.begin(); }
        typename std::vector<T>::iterator end() { return ArrayInternal.end(); }
        typename std::vector<T>::const_iterator begin() const { return ArrayInternal.cbegin(); }
        typename std::vector<T>::const_iterator end() const { return ArrayInternal.cend(); }

        bool Contains(const T& value) const
        {
            return std::find(ArrayInternal.begin(), ArrayInternal.end(), value) != ArrayInternal.end();
        }

        void Sort()
        {
            std::sort(ArrayInternal.begin(), ArrayInternal.end());
        }

        template <typename Compare>
        void Sort(Compare comp)
        {
            std::sort(ArrayInternal.begin(), ArrayInternal.end(), comp);
        }

        void Print() const
        {
            for (const auto& elem : ArrayInternal)
            {
                WD_CORE_INFO(elem);
            }
        }
    };
}