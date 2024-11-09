#pragma once

namespace Waldem
{
    enum class ShaderDataType
    {
        None = 0,
        Float = 1,
        Float2 = 2,
        Float3 = 3,
        Float4 = 4,
        Mat3 = 5,
        Mat4 = 6,
        Int = 7,
        Int2 = 8,
        Int3 = 9,
        Int4 = 10,
        Bool = 11,
    };

    static int16_t ShaderDataTypeSize(ShaderDataType type)
    {
        switch (type)
        {
            case ShaderDataType::None:  return 0;
            case ShaderDataType::Float: return 4;
            case ShaderDataType::Float2:    return 8;
            case ShaderDataType::Float3:    return 12;
            case ShaderDataType::Float4:    return 16;
            case ShaderDataType::Mat3:  return 36;
            case ShaderDataType::Mat4:  return 64;
            case ShaderDataType::Int:   return 4;
            case ShaderDataType::Int2:  return 8;
            case ShaderDataType::Int3:  return 12;
            case ShaderDataType::Int4:  return 16;
            case ShaderDataType::Bool:  return 1;
        }

        WD_CORE_ASSERT(false, "Uknown shader data type")

        return 0;
    }
    
    struct BufferElement
    {
        ShaderDataType Type;
        String Name;
        uint32_t Size;
        uint32_t Offset;
        bool Normalized;

        BufferElement() {}
        BufferElement(ShaderDataType type, const String& name, bool normalized) : Type(type), Name(name), Size(ShaderDataTypeSize(type)), Offset(0), Normalized(normalized)
        {
        }

        int GetComponentCount() const
        {
            switch (Type)
            {
                case ShaderDataType::None: return 0;
                case ShaderDataType::Float: return 1;
                case ShaderDataType::Float2:    return 2;
                case ShaderDataType::Float3:    return 3;
                case ShaderDataType::Float4:    return 4;
                case ShaderDataType::Mat3:  return 9;
                case ShaderDataType::Mat4:  return 16;
                case ShaderDataType::Int:   return 1;
                case ShaderDataType::Int2:  return 2;
                case ShaderDataType::Int3:  return 3;
                case ShaderDataType::Int4:  return 4;
                case ShaderDataType::Bool:  return 1;
            }

            WD_CORE_ASSERT(false, "Uknown shader data type")

            return 0;
        }
    };
        
    class BufferLayout
    {
    public:
        BufferLayout() {}
        BufferLayout(const std::initializer_list<BufferElement>& elements) : Elements(elements)
        {
            CalculateOffsetsAndStride();
        }
        
        const std::vector<BufferElement>& GetElements() const { return Elements; }
        const int GetStride() const { return Stride; }

        std::vector<BufferElement>::iterator begin() { return Elements.begin(); }
        std::vector<BufferElement>::iterator end() { return Elements.end(); }
        std::vector<BufferElement>::const_iterator begin() const { return Elements.begin(); }
        std::vector<BufferElement>::const_iterator end() const { return Elements.end(); }
    private:
        void CalculateOffsetsAndStride()
        {
            uint32_t offset = 0;
            Stride = 0;
            
            for (auto& element : Elements)
            {
                element.Offset = offset;
                offset += element.Size;
                Stride += element.Size;
            }
        }
        
        std::vector<BufferElement> Elements;
        int Stride = 0;
    };
    
    class VertexBuffer
    {
    public:
        virtual ~VertexBuffer() {}
    };
    
    class IndexBuffer
    {
    public:
        virtual ~IndexBuffer() {}
        virtual uint32_t GetCount() const = 0;

        void SetIndices(uint32_t* indices) { Indices = indices; }

        uint32_t* Indices;
    };

    class StorageBuffer
    {
    public:
        virtual ~StorageBuffer() {}

        virtual void* GetPlatformResource() const = 0;
        
        static StorageBuffer* Create(void* data, size_t size);
    };
}
