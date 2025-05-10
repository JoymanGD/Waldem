#pragma once

namespace Waldem
{
    struct WDataBuffer
    {
    private:
        unsigned char* Data = nullptr;
        size_t Size = 0;
	    size_t Cursor = 0;

    public:

	    FORCEINLINE void* GetData() const { return Data; }
	    FORCEINLINE size_t GetSize() const { return Size; }
	    FORCEINLINE size_t Tell() const { return Cursor; }

        FORCEINLINE auto GetDataCopy() const
        {
            unsigned char* data = new unsigned char[Size];
            memcpy(data, Data, Size);
            return std::move(data);
        }

        WDataBuffer() = default;

        WDataBuffer(const void* data, const size_t size) : Size(size)
        {
            Data = new unsigned char[size];
            memcpy(Data, data, size);
        }

        WDataBuffer(const WDataBuffer& other) : Size(other.Size), Cursor(other.Cursor)
        {
            Data = new unsigned char[Size];
            memcpy(Data, other.Data, Size);
        }

        WDataBuffer& operator=(const WDataBuffer& other)
        {
            if (this != &other)
            {
                delete[] Data;
                Size = other.Size;
                Cursor = other.Cursor;
                Data = new unsigned char[Size];
                memcpy(Data, other.Data, Size);
            }
            return *this;
        }

	    size_t Read(void* buffer, const size_t size, const size_t count = 1)
	    {
		    size_t read = size * count;
		    if (Cursor + read > Size)
		    {
			    read = Size - Cursor;
		    }

		    if (buffer == nullptr)
		    {
			    buffer = new unsigned char[read];
		    }

		    memcpy(buffer, Data + Cursor, read);
		    Cursor += read;
		    return read;
	    }

        void Write(const void* data, size_t size)
        {
            unsigned char* newData = new unsigned char[Size + size];
            memcpy(newData, Data, Size);
            memcpy((newData + Size), data, size);
            delete[] Data;
            Data = newData;
            Size += size;
        }

        void AddString(const char* string)
        {
            Write(string, strlen(string));
        }

        // Overloading >> for reading
        template <typename T>
        WDataBuffer& operator>>(T& value)
        {
            Read(&value, sizeof(T));
            return *this;
        }

        // Overloading << for writing
        template <typename T>
        WDataBuffer& operator<<(const T& value)
        {
            Write(&value, sizeof(T));
            return *this;
        }

        // Overloading << for strings
        WDataBuffer& operator<<(const char* str)
        {
            AddString(str);
            return *this;
        }

        // Overloading >> for strings (assumes reading into a fixed buffer)
        WDataBuffer& operator>>(char* str)
        {
            size_t length = strlen((char*)Data + Cursor) + 1;
            Read(str, length);
            return *this;
        }

        void Prepend(const void* data, size_t size)
        {
            unsigned char* newData = new unsigned char[Size + size];
            memcpy(newData, data, size);             // Copy new data to the beginning
            memcpy(newData + size, Data, Size);      // Append existing data after it
            delete[] Data;
            Data = newData;
            Size += size;
            Cursor += size; // Optional: move cursor forward if it should remain at the same logical place
        }

        ~WDataBuffer()
        {
            delete[] Data;
        }
    };
}
