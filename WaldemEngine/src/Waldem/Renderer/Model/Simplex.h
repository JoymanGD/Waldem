#pragma once

namespace Waldem
{
    struct SimplexVertex
    {
        Vector3 Point;
        Vector3 SupportA;
        Vector3 SupportB;

        SimplexVertex() : Point(Vector3(0)), SupportA(Vector3(0)), SupportB(Vector3(0)) {}
        SimplexVertex(Vector3 a, Vector3 b) : Point(a - b), SupportA(a), SupportB(b) {}
    };
    
    struct Simplex
    {
    private:
        std::array<SimplexVertex, 4> Points;
        int Size;

    public:
        Simplex() : Points({SimplexVertex(), SimplexVertex(), SimplexVertex(), SimplexVertex()}), Size(0) {}

        Simplex& operator=(std::initializer_list<SimplexVertex> list)
        {
            for(auto v = list.begin(); v != list.end(); v++)
            {
                Points[std::distance(list.begin(), v)] = *v;
            }

            Size = list.size();

            return *this;
        }

        void Add(const SimplexVertex& point)
        {
            Points = { point, Points[0], Points[1], Points[2] };
            Size = std::min(Size + 1, 4);
        }

        SimplexVertex& operator[](int i) { return Points[i]; }

        int Num() const { return Size; }

        auto begin() const { return Points.begin(); }
        auto end() const { return Points.end() - (4 - Size); }
    };
}