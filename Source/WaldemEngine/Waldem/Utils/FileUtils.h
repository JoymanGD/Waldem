#pragma once
#include <commdlg.h>
#include <string>
#include <filesystem>
#include <shobjidl.h>
#include <objbase.h>
#include "Waldem/Window.h"

namespace Waldem
{
    inline Path GetCurrentFolder()
    {
        char buffer[MAX_PATH];
        GetModuleFileNameA(nullptr, buffer, MAX_PATH);
        return Path(buffer).parent_path();
    }

    inline bool SaveFile(Path& outPath, const WString& extension, WString filter)
    {
        OPENFILENAMEW ofn;
        wchar_t szFile[MAX_PATH] = L"";

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = CWindow::Instance->GetWindowsHandle();
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;
        // ofn.lpstrFilter = L"Scene Files (*.scene)\0*.scene\0All Files (*.*)\0*.*\0";
        ofn.lpstrFilter = filter.ToWString().c_str();
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

        if (GetSaveFileNameW(&ofn))
        {
            Path selectedPath = szFile;

            if (selectedPath.extension().string() != extension.GetData())
            {
                selectedPath.replace_extension(extension.GetData());
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
        ofn.lpstrFilter = L"Scene Files (*.scene)\0*.scene\0All Files (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileNameW(&ofn))
        {
            outPath = Path(szFile);
            return true;
        }

        return false;
    }

    inline bool SelectFolder(Path& outPath)
    {
        IFileDialog* dialog = nullptr;

        HRESULT hr = CoCreateInstance(
            CLSID_FileOpenDialog,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&dialog));

        if (FAILED(hr))
            return false;

        DWORD options = 0;
        dialog->GetOptions(&options);
        dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);

        bool success = false;

        if (SUCCEEDED(dialog->Show(CWindow::Instance->GetWindowsHandle())))
        {
            IShellItem* item = nullptr;

            if (SUCCEEDED(dialog->GetResult(&item)))
            {
                PWSTR folderPath = nullptr;

                if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &folderPath)))
                {
                    outPath = Path(folderPath);
                    CoTaskMemFree(folderPath);
                    success = true;
                }

                item->Release();
            }
        }

        dialog->Release();
        return success;
    }
}