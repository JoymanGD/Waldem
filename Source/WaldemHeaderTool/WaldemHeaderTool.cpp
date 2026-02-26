#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

struct Field
{
    std::string Type;
    std::string Name;
    bool Hidden = false;
    std::string ExplicitRegisterType;
};

struct Component
{
    std::string Name;
    std::vector<Field> Fields;
    bool EditorOnly = false;
    bool HiddenComponent = false;
};

bool IsAssetReferenceType(const std::string& type)
{
    return type.size() >= 9 &&
           type.substr(type.size() - 9) == "Reference";
}

static std::string Trim(const std::string& s)
{
    const char* ws = " \t\n\r";
    size_t start = s.find_first_not_of(ws);
    size_t end = s.find_last_not_of(ws);
    if (start == std::string::npos)
        return "";
    return s.substr(start, end - start + 1);
}

void GenerateComponent(const Component& comp, const fs::path& outputDir)
{
    fs::create_directories(outputDir);
    fs::path outputFile = outputDir / (comp.Name + ".generated.h");
    std::ofstream out(outputFile);

    out << "// Auto-generated. Do not modify.\n";
    out << "#pragma once\n\n";
    out << "#include \"Waldem/ECS/ECS.h\"\n\n";
    out << "namespace Waldem\n{\n\n";

    out << "    inline void RegisterComponent_" << comp.Name << "(flecs::world& world)\n";
    out << "    {\n";
    out << "        auto component = world.component<" << comp.Name << ">()";

    if (!comp.Fields.empty() || comp.EditorOnly || comp.HiddenComponent)
        out << "\n";

    for (const auto& field : comp.Fields)
    {
        std::string fieldName = field.Hidden ? "___" + field.Name : field.Name;

        out << "            ";

        if (!field.ExplicitRegisterType.empty())
        {
            out << ".member<" << field.ExplicitRegisterType
                << ">(\"" << fieldName << "\")";
        }
        else if (IsAssetReferenceType(field.Type))
        {
            out << ".member<AssetReference>(\""
                << fieldName << "\")";
        }
        else
        {
            out << ".member(\""
                << fieldName
                << "\", &"
                << comp.Name
                << "::"
                << field.Name
                << ")";
        }

        out << "\n";
    }

    if (comp.EditorOnly)
        out << "            .add<EditorComponent>()\n";

    if (comp.HiddenComponent)
        out << "            .add<HiddenComponent>()\n";

    out << "        ;\n\n";

    // 🔥 THIS IS THE IMPORTANT PART
    // Only register non-hidden components in editor list
    if (!comp.HiddenComponent)
    {
        out << "        ECS::RegisteredComponents.Add(WString(component.name().c_str()), component);\n";
    }

    out << "    }\n\n";

    out << "    struct AutoRegister_" << comp.Name << "\n";
    out << "    {\n";
    out << "        AutoRegister_" << comp.Name << "()\n";
    out << "        {\n";
    out << "            ECS::RegisterComponent(&RegisterComponent_" << comp.Name << ");\n";
    out << "        }\n";
    out << "    };\n\n";

    out << "    inline AutoRegister_" << comp.Name
        << " _autoRegister_" << comp.Name << ";\n\n";

    out << "} // namespace Waldem\n";

    std::cout << "Generated: " << outputFile << "\n";
}

void ProcessComponents(const std::string& content, const fs::path& outputDir)
{
    std::regex componentRegex(
        R"(COMPONENT\(([^)]*)\)\s*struct\s+(?:\w+\s+)*(\w+)\s*\{)");

    auto compBegin = std::sregex_iterator(content.begin(), content.end(), componentRegex);
    auto compEnd = std::sregex_iterator();

    for (auto it = compBegin; it != compEnd; ++it)
    {
        Component comp;
        comp.Name = (*it)[2];

        std::string componentArgs = (*it)[1];

        if (componentArgs.find("EditorOnly") != std::string::npos)
            comp.EditorOnly = true;

        if (componentArgs.find("Hidden") != std::string::npos)
            comp.HiddenComponent = true;

        size_t structStart = content.find("{", it->position());
        if (structStart == std::string::npos)
            continue;

        int braceDepth = 1;
        size_t pos = structStart + 1;

        while (pos < content.size() && braceDepth > 0)
        {
            if (content[pos] == '{') braceDepth++;
            if (content[pos] == '}') braceDepth--;
            pos++;
        }

        if (braceDepth != 0)
            continue;

        std::string structBody =
            content.substr(structStart + 1, pos - structStart - 2);

        std::stringstream bodyStream(structBody);
        std::string line;

        bool expectField = false;
        Field currentField;

        while (std::getline(bodyStream, line))
        {
            line = Trim(line);
            if (line.empty())
                continue;

            std::smatch fieldMacroMatch;
            std::regex fieldMacro(R"(FIELD(?:\(([^)]*)\))?)");

            if (std::regex_search(line, fieldMacroMatch, fieldMacro))
            {
                expectField = true;
                currentField = Field{};
                std::string args = fieldMacroMatch[1];

                if (args.find("Hidden") != std::string::npos)
                    currentField.Hidden = true;

                std::smatch typeMatch;
                std::regex typeArg(R"(Type\s*=\s*([\w:<>]+))");
                if (std::regex_search(args, typeMatch, typeArg))
                    currentField.ExplicitRegisterType = typeMatch[1];

                continue;
            }

            if (!expectField)
                continue;

            expectField = false;

            if (line.find('(') != std::string::npos)
                continue;

            std::regex fieldRegex(
                R"(^([\w:<>]+)\s+(\w+)\s*(=.*)?;)");

            std::smatch fieldMatch;
            if (std::regex_match(line, fieldMatch, fieldRegex))
            {
                currentField.Type = fieldMatch[1];
                currentField.Name = fieldMatch[2];
                comp.Fields.push_back(currentField);
            }
        }

        GenerateComponent(comp, outputDir);
    }
}

void ProcessFile(const fs::path& filePath, const fs::path& outputDir)
{
    std::ifstream file(filePath);
    if (!file.is_open())
        return;

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    ProcessComponents(content, outputDir);
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cout << "Usage: WaldemHeaderTool <input_directory> <output_directory>\n";
        return 1;
    }

    fs::path inputDir  = fs::path(argv[1]).lexically_normal();
    fs::path outputDir = fs::path(argv[2]).lexically_normal();

    for (auto& entry : fs::recursive_directory_iterator(inputDir))
    {
        if (entry.path().extension() == ".h")
        {
            ProcessFile(entry.path(), outputDir);
        }
    }

    return 0;
}