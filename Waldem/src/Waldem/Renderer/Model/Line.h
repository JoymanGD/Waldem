#pragma once

namespace Waldem
{
    struct LineVertex
    {
        Vector4 Position;
        Vector4 Color;
    };
    
    struct Line
    {
        Line(Vector3 start, Vector3 end, Vector4 color)
        {
            Start.Color = color;
            Start.Position = Vector4(start, 1.0f);
            End.Color = color;
            End.Position = Vector4(end, 1.0f);
        }

        void ToClipSpace(Matrix4 matrix)
        {
            Start.Position = matrix * Start.Position;
            End.Position = matrix * End.Position;
        }
        
        LineVertex Start;
        LineVertex End;
    };
}