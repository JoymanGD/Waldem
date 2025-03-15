#pragma once

namespace Waldem
{
    struct Simplex
    {
    private:
        std::array<Vector3, 4> Points;
        unsigned Size;

    public:
        Simplex() : Points({Vector3(0), Vector3(0), Vector3(0), Vector3(0)}), Size(0) {}

        Simplex& operator=(std::initializer_list<Vector3> list)
        {
            for(auto v = list.begin(); v != list.end(); v++)
            {
                Points[std::distance(list.begin(), v)] = *v;
            }

            Size = list.size();

            return *this;
        }

        void Add(const Vector3& point)
        {
            Points = { point, Points[0], Points[1], Points[2] };
            Size = std::min(Size + 1, 4u);
        }

        Vector3& operator[](unsigned i) { return Points[i]; }

        unsigned Num() const { return Size; }

        auto begin() const { return Points.begin(); }
        auto end() const { return Points.end() - (4 - Size); }
    };
}