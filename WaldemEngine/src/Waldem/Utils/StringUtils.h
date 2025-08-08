#pragma once
#include "Waldem/Types/String.h"

namespace Waldem
{
    inline WString SanitizeString(WString& input)
    {
        static const std::unordered_set invalidChars{
            '<','>',':','"','/','\\','|','?','*','[',']'
        };

        std::string s = input.ToString();
        std::string out;
        out.reserve(s.size());

        for (char c : s)
        {
            if (invalidChars.count(c) > 0)
                continue; // skip entirely

            if (c == '-')
                out.push_back('_');
            else
                out.push_back(c);
        }

        return WString(out.c_str()); // calls SetData & updates hash
    }
}
