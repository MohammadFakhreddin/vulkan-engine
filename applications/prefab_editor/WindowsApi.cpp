#include "WindowsApi.hpp"

#include <algorithm>
#include <Windows.h>
#include <shobjidl.h>
#include <vector>

bool WinApi::TryToPickFile(std::string & outFileAddress)
{
    //https://docs.microsoft.com/en-us/windows/win32/learnwin32/example--the-open-dialog-box
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileOpenDialog * pFileOpen;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(
            CLSID_FileOpenDialog,
            nullptr,
            CLSCTX_ALL,
            IID_IFileOpenDialog,
            reinterpret_cast<void **>(&pFileOpen)
        );

        std::vector<COMDLG_FILTERSPEC> const acceptedFileTypes{
            {
                L"Gltf files",
                L"*.gltf"
            },
            {
                L"Glb files",
                L"*.glb"
            }
        };
        pFileOpen->SetFileTypes(static_cast<UINT>(acceptedFileTypes.size()), acceptedFileTypes.data());

        if (SUCCEEDED(hr))
        {
            // Show the Open dialog box.
            hr = pFileOpen->Show(nullptr);

            // Get the file name from the dialog box.
            if (SUCCEEDED(hr))
            {
                IShellItem * pItem;
                hr = pFileOpen->GetResult(&pItem);
                if (SUCCEEDED(hr))
                {
                    PWSTR pszFilePath;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                    // Display the file name to the user.
                    if (SUCCEEDED(hr))
                    {
                        //MessageBoxW(NULL, pszFilePath, L"File Path", MB_OK);
                        std::wstring wFilePath = pszFilePath;
                        //outFileAddress = std::string( wFilePath.begin(), wFilePath.end() );
                        outFileAddress = std::string(wFilePath.length(), 0);
                        // Microsoft is a retarded company!
                        std::ranges::transform(wFilePath, outFileAddress.begin(), [](wchar_t c)->char
                        {
                            return static_cast<char>(c);
                        });
                        CoTaskMemFree(pszFilePath);
                        return true;
                    }
                    pItem->Release();
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
    }
    return false;
}
