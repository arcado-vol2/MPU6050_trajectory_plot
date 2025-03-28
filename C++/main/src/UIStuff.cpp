#include "UIStuff.h"

#include <string>
#include <Windows.h>
#include <ShObjIdl.h>
#include "imgui.h"

namespace UIStuff {
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


    std::string OpenFileDialog(const std::wstring& filter)
    {
        std::string filePath;

        CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        IFileDialog* pFileDialog;

        if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
            IID_IFileDialog, (void**)&pFileDialog)))
        {
            // Устанавливаем фильтр файлов
            COMDLG_FILTERSPEC fileTypes[] = {
                { L"Supported Files", filter.c_str() }
            };
            pFileDialog->SetFileTypes(1, fileTypes);

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
                        filePath = buffer;
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFileDialog->Release();
        }
        CoUninitialize();

        return filePath;
    }




    void PopUp::ShowPopUp(const std::string& label, const std::string& message) {
        popUpMessage = message;
        popUpLabel = label;
        isPopUpShowing = true;
    }

    void PopUp::HidePopUp() {
        ImGui::CloseCurrentPopup();
        isPopUpShowing = false;
    }

    void PopUp::RenderPopUp() {
        if (isPopUpShowing) {
            ImGui::OpenPopup(popUpLabel.c_str());
            if (ImGui::BeginPopupModal(popUpLabel.c_str(), &isPopUpShowing, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text(popUpLabel.c_str());
                ImGui::TextWrapped("%s", popUpMessage.c_str());
                ImGui::Separator();

                if (ImGui::Button("OK", ImVec2(120, 0))) {
                    HidePopUp();
                }

                ImGui::EndPopup();
            }
        }
    }
}