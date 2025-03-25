#include "FileDialog.h"

#include <string>
#include <Windows.h>
#include <ShObjIdl.h>

std::string OpenFolderDialog()
{
    std::string folderPath;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    IFileDialog* pFileDialog;

    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
        IID_IFileDialog, (void**)&pFileDialog)))
    {
        DWORD dwOptions;
        if (SUCCEEDED(pFileDialog->GetOptions(&dwOptions)))
        {
            pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS);
        }

        if (SUCCEEDED(pFileDialog->Show(NULL)))
        {
            IShellItem* pItem;
            if (SUCCEEDED(pFileDialog->GetResult(&pItem)))
            {
                PWSTR pszFilePath;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
                {
                    char buffer[MAX_PATH];
                    WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, buffer, MAX_PATH, NULL, NULL);
                    folderPath = buffer;
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileDialog->Release();
    }
    CoUninitialize();

    return folderPath;
}