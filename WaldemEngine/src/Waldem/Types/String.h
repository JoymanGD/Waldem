#pragma once

#include "WArray.h"
#include "DataBuffer.h"
#include "Waldem/Serialization/Serializable.h"
#include <cstring>

namespace Waldem
{
	enum EStringComparison
	{
		StringComparison_CaseSensitive,
		StringComparison_CaseInsensitive,
		StringComparison_Numeric
	};

	FORCEINLINE constexpr long strhash(const char* string) { return (*(long*)string); }

	FORCEINLINE constexpr bool faststrcmp(const char* string1, const char* string2)
	{
	    while (*string1 && (*string1 == *string2))
	    {
	        ++string1;
	        ++string2;
	    }
	    return *string1 == *string2;
	}

	struct WString: public ISerializable
	{
	private:
	    char* Data = nullptr;
	    uint StringHash = 0;

	    FORCEINLINE constexpr void UpdateHash()
	    {
	        StringHash = Hash(Data);
	    }

	    FORCEINLINE constexpr void SetData(const char* newData)
	    {
	        if (Data)
	        {
	            delete[] Data;
	        }

	        likely_if (newData)
	        {
	            Data = new char[strlen(newData) + 1];
	            strcpy(Data, newData);
	            UpdateHash();
	        }
	        unlikely_else
	        {
	            Data = nullptr;
	            StringHash = 0;
	        }
	    }

	public:

		char* GetData() const { return Data; }

		size_t GetSize() const { return Data ? strlen(Data) : 0; }

		std::string ToString() const { return Data ? std::string(Data) : ""; }

	    void Serialize(WDataBuffer& outData) final
		{
	        size_t len = Length();
	        outData.Write(&len, sizeof(size_t));
	        if(len > 0)
	            outData.Write(Data, len);
		}

		void Deserialize(WDataBuffer& inData) final
	    {
	        size_t len = 0;
	        inData.Read(&len, sizeof(size_t));
	        if(len > 0)
	        {
	            char* newData = new char[len + 1];
	            newData[len] = '\0';
	            inData.Read(newData, len);
	            SetData(newData);
	            delete[] newData;
	        }
	    }

	    FORCEINLINE constexpr static uint Hash(const char* str) noexcept
	    {
	        uint hash = 0;
	        while (*str)
	        {
	            hash = (hash * 31) + static_cast<uint>(*str);
	            ++str;
	        }
	        return hash;
	    }

	    FORCEINLINE constexpr uint GetHash() const noexcept { return StringHash; }

	    // Default constructor
	    FORCEINLINE constexpr WString() = default;

		FORCEINLINE constexpr bool IsEmpty() const { return Data == nullptr || strlen(Data) == 0; }

		FORCEINLINE constexpr bool IsValid() const { return Data != nullptr; }

		FORCEINLINE constexpr bool operator!() const { return Data == nullptr; }

	    FORCEINLINE constexpr const char* operator*() const { return Data ? Data : ""; }

	    FORCEINLINE constexpr operator const char*() const { return Data ? Data : ""; }

	    FORCEINLINE uint operator()(const WString& str) const noexcept
	    {
	        return str.StringHash;
	    }

	    // Utility: Get raw C-string
	    FORCEINLINE constexpr const char* C_Str() const { return Data ? Data : ""; }

	    // Constructor from C-string
	    FORCEINLINE constexpr WString(const char* data)
	    {
	        SetData(data);
	    }
		
		// Constructor from C-string
		FORCEINLINE constexpr WString(std::string string)
		{
			SetData(string.c_str());
		}

	    // Copy constructor (deep copy)
	    FORCEINLINE constexpr WString(const WString& other)
	    {
	        SetData(other.Data);
	    }

	    // Move constructor
	    FORCEINLINE constexpr WString(WString&& other) noexcept: Data(other.Data) { other.Data = nullptr; }

	    // Destructor
	    ~WString()
	    {
	        delete[] Data;
	    }

		// Clears the string data
		FORCEINLINE void Clear()
		{
		    delete[] Data;
		    Data = nullptr;
		    StringHash = 0;
		}

		// Converts an integer to a WString
		static WString FromInt(int value)
		{
		    char buffer[12];
		    snprintf(buffer, sizeof(buffer), "%d", value);
		    return WString(buffer);
		}
		
		// Converts a float to a WString
		static WString FromFloat(float value, const char* format = "%.6f")
		{
		    char buffer[32];
		    snprintf(buffer, sizeof(buffer), format, value);
		    return WString(buffer);
		}

		// Implicit conversion to C-string

		// Compares this string with another (Case-insensitive by default)
		bool Equals(const WString& other, EStringComparison comparison = StringComparison_CaseInsensitive) const
		{
			if (comparison == StringComparison_CaseSensitive)
			{
				return faststrcmp(Data ? Data : "", other.Data ? other.Data : "");
			}
			else if (comparison == StringComparison_CaseInsensitive)
			{
				return ToLower() == other.ToLower();
			}
			else if (comparison == StringComparison_Numeric)
			{
				return atof(Data ? Data : "") == atof(other.Data ? other.Data : "");
			}

			return false;
		};

	    // Copy assignment operator
	    FORCEINLINE constexpr WString& operator=(const WString& other)
	    {
	        if (this != &other)
	        {
	            SetData(other.Data);
	        }
	        return *this;
	    }

	    // Move assignment operator
	    FORCEINLINE constexpr WString& operator=(WString&& other) noexcept
	    {
	        if (this != &other)
	        {
	            delete[] Data;
	            Data = other.Data;
	            StringHash = other.StringHash;
	            other.Data = nullptr;
	            other.StringHash = 0;
	        }
	        return *this;
	    }

	    // Assign from C-string
	    FORCEINLINE constexpr WString& operator=(const char* other)
	    {
	        SetData(other);
	        return *this;
	    }

	    // Concatenation operator
	    WString operator+(const WString& other) const
	    {
	        uint len1 = Data ? strlen(Data) : 0;
	        uint len2 = other.Data ? strlen(other.Data) : 0;

	        char* buffer = new char[len1 + len2 + 1];
	        if (Data) strcpy(buffer, Data);
	        if (other.Data) strcat(buffer, other.Data);

	        WString result(buffer);
	        delete[] buffer; // Prevent memory leaks
	        return result;
	    }

	    // Append operator
	    FORCEINLINE constexpr WString& operator+=(const WString& other)
	    {
	        uint len1 = Data ? strlen(Data) : 0;
	        uint len2 = other.Data ? strlen(other.Data) : 0;

	        char* buffer = new char[len1 + len2 + 1];
			buffer[0] = '\0';
	        if (Data) strcpy(buffer, Data);
	        if (other.Data) strcat(buffer, other.Data);

	        delete[] Data;
	        Data = buffer;
	        UpdateHash();
	        return *this;
	    } 

	    // Comparison operators
	    FORCEINLINE constexpr bool operator==(const WString& other) const
	    {
	        return faststrcmp(Data ? Data : "", other.Data ? other.Data : "");
	    }

		FORCEINLINE constexpr bool operator==(const char* other) const
		{
			return faststrcmp(Data ? Data : "", other ? other : "");
		}

	    FORCEINLINE constexpr bool operator!=(const WString& other) const
	    {
	        return !(*this == other);
	    }

		// Get length of the string
	    FORCEINLINE constexpr uint Length() const
	    {
	        return Data ? strlen(Data) : 0;
	    }

		// Appends another string to this one
	    FORCEINLINE constexpr void Append(const WString& other)
	    {
	        *this += other;
	    }

	    // Convert to uppercase in-place
	    void MakeUpper()
	    {
	        if (Data)
	        {
	            for (uint i = 0; Data[i]; ++i)
	            {
	                Data[i] = static_cast<char>(toupper(Data[i]));
	            }
	            UpdateHash();
	        }
	    }

	    // Convert to lowercase in-place
	    void MakeLower()
	    {
	        if (Data)
	        {
	            for (uint i = 0; Data[i]; ++i)
	            {
	                Data[i] = static_cast<char>(tolower(Data[i]));
	            }
	            UpdateHash();
	        }
	    }

	    // Convert to uppercase and return a new string
	    WString ToUpper() const
	    {
	        WString result(*this);
	        result.MakeUpper();
	        return result;
	    }

	    // Convert to lowercase and return a new string
	    WString ToLower() const
	    {
	        WString result(*this);
	        result.MakeLower();
	        return result;
	    }

	    // Get substring
		WString Substr(uint start) const
	    {
	        if (!Data) return WString();

	        uint len = strlen(Data);
	        if (start >= len) return WString();

	        char* buffer = new char[len - start + 1];
	        strcpy(buffer, Data + start);

	        WString result(buffer);
	        delete[] buffer;
	        return result;
	    }

		// Get substring
		WString Substr(uint start, uint count) const
		{
			if (!Data) return WString();

			uint len = strlen(Data);
			if (start >= len) return WString();

			char* buffer = new char[count + 1];
			strncpy(buffer, Data + start, count);
			buffer[count] = '\0';

			WString result(buffer);
			delete[] buffer;
			return result;
		}

		// Find first occurrence of a character
		uint Find(char c) const
		{
			if (!Data) return SIZE_MAX;

			char* ptr = strchr(Data, c);
			return ptr ? ptr - Data : SIZE_MAX;
		}

		// Find first occurrence of a substring
		uint Find(const WString& other) const
		{
			if (!Data || !other.Data) return SIZE_MAX;

			char* ptr = strstr(Data, other.Data);
			return ptr ? ptr - Data : SIZE_MAX;
		}

		// Replace all occurrences of a character
		void ReplaceAll(char c, char newChar)
		{
			if (Data)
			{
				for (uint i = 0; Data[i]; ++i)
				{
					if (Data[i] == c)
					{
						Data[i] = newChar;
					}
				}
	            UpdateHash();
			}
		}

		// Replace all occurrences of a substring
		void ReplaceAll(const WString& oldStr, const WString& newStr)
		{
			if (!Data || !oldStr.Data) return;

			uint oldLen = strlen(oldStr.Data);
			uint newLen = strlen(newStr.Data);

			uint pos = 0;
			while ((pos = Find(oldStr)) != SIZE_MAX)
			{
				char* buffer = new char[Length() - oldLen + newLen + 1];
				strncpy(buffer, Data, pos);
				buffer[pos] = '\0';
				strcat(buffer, newStr.Data);
				strcat(buffer, Data + pos + oldLen);

				delete[] Data;
				Data = buffer;
			}

	        UpdateHash();
		}

		// Split string into tokens
		WArray<WString> Split(char delimiter) const
		{
			WArray<WString> tokens;

			if (!Data) return tokens;

			uint len = strlen(Data);
			uint start = 0;
			for (uint i = 0; i < len; ++i)
			{
				if (Data[i] == delimiter)
				{
					tokens.Add(Substr(start, i - start));
					start = i + 1;
				}
			}

			if (start < len)
			{
				tokens.Add(Substr(start, len - start));
			}

			return tokens;
		}

	    bool IsOneOf(const WArray<WString>& strings) const
	    {
	        for (const WString& str : strings)
	        {
	            if (Equals(str))
	            {
	                return true;
	            }
	        }
	        return false;
	    }

		// Get pointer to the first character
		const char* Begin() const {	return Data; }

		// Get pointer to the null terminator
		const char* End() const	{ return Data ? Data + strlen(Data) : nullptr; }

		// Get pointer to the last character
		const char* Last() const { return Data ? Data + strlen(Data) - 1 : nullptr; }

		// Get pointer to the null terminator
		const char* CEnd() const { return Data ? Data + strlen(Data) : nullptr; }

		// Find last occurrence of
		WString FindLast(char c) const
		{
			if (!Data) return WString();

			char* ptr = strrchr(Data, c);
			return ptr ? WString(ptr) : WString();
		}

		// Find last occurrence of a substring
		WString FindLast(const WString& other) const
		{
			if (!Data || !other.Data) return WString();

			char* ptr = strstr(Data, other.Data);
			return ptr ? WString(ptr) : WString();
		}

	    FORCEINLINE WString GetFileExtension() const { return FindLast('.').Substr(1).ToLower(); }
	};

	using WStringList = WArray<WString>;
}

namespace std {
	template<>
	struct hash<Waldem::WString>
	{
		Waldem::uint operator()(const Waldem::WString& str) const noexcept
		{
			return str.GetHash();
		}
	};
}
