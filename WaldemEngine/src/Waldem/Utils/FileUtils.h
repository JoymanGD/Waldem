#pragma once
#include <windows.h>
#include <commdlg.h>
#include <string>
#include <filesystem>
#include "Waldem/Window.h"

namespace Waldem
{
    inline Path GetCurrentFolder()
    {
        char buffer[MAX_PATH];
        GetModuleFileNameA(nullptr, buffer, MAX_PATH);
        return Path(buffer).parent_path();
    }

    inline bool SaveFile(Path& outPath)
    {
        OPENFILENAMEW ofn;
        wchar_t szFile[MAX_PATH] = L"";

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = CWindow::Instance->GetWindowsHandle();
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = L"Asset Files (*.ass)\0*.ass\0All Files (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

        if (GetSaveFileNameW(&ofn))
        {
            Path selectedPath = szFile;

            if (selectedPath.extension() != L".ass")
            {
                selectedPath += L".ass";
            }

            outPath = selectedPath;
            return true;
        }

        return false;
    }

    inline bool OpenFile(Path& outPath)
    {
        OPENFILENAMEW ofn;
        wchar_t szFile[MAX_PATH] = L"";

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = CWindow::Instance->GetWindowsHandle();
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = L"Asset Files (*.ass)\0*.ass\0All Files (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileNameW(&ofn))
        {
            outPath = Path(szFile);
            return true;
        }

        return false;
    }
}