#include "WindowsApi.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <ShObjidl.h>

#include <algorithm>
#include <codecvt>
#include <vector>

//-------------------------------------------------------------------------------------------------

static std::wstring s2ws(const std::string& s)
{
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0); 
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
}

//-------------------------------------------------------------------------------------------------

bool WinApi::TryToPickFile(
    std::vector<Extension> const & extensions,
    std::string & outFileAddress
)
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

        /*std::vector<COMDLG_FILTERSPEC> const acceptedFileTypes{
            {
                L"Gltf files",
                L"*.gltf"
            },
            {
                L"Glb files",
                L"*.glb"
            }
        };*/

        std::vector<COMDLG_FILTERSPEC> acceptedFileTypes{};

        std::vector<std::wstring> wStrings (extensions.size() * 2);

        for (int i = 0; i < static_cast<int>(extensions.size()); ++i)
        {
            acceptedFileTypes.emplace_back();

            wStrings[i * 2] = s2ws(extensions[i].name);
            acceptedFileTypes.back().pszName = wStrings[i * 2].data();
            wStrings[i * 2 + 1] = s2ws(extensions[i].value);
            acceptedFileTypes.back().pszSpec = wStrings[i * 2  + 1].data();
        }

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

//-------------------------------------------------------------------------------------------------

bool WinApi::SaveAs(
    std::vector<Extension> const & extensions,
    std::string & outFilePath
)
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileOpenDialog * pFileOpen;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(
            CLSID_FileSaveDialog,
            nullptr,
            CLSCTX_ALL,
            IID_IFileSaveDialog,
            reinterpret_cast<void **>(&pFileOpen)
        );

        /*std::vector<COMDLG_FILTERSPEC> const acceptedFileTypes{
            {
                L"Json",
                L"*.json"
            }
        };*/

        std::vector<COMDLG_FILTERSPEC> acceptedFileTypes{};

        std::vector<std::wstring> wStrings (extensions.size() * 2);

        for (int i = 0; i < static_cast<int>(extensions.size()); ++i)
        {
            acceptedFileTypes.emplace_back();

            wStrings[i * 2] = s2ws(extensions[i].name);
            acceptedFileTypes.back().pszName = wStrings[i * 2].data();
            wStrings[i * 2 + 1] = s2ws(extensions[i].value);
            acceptedFileTypes.back().pszSpec = wStrings[i * 2  + 1].data();
        }

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
                        outFilePath = std::string(wFilePath.length(), 0);
                        // Microsoft is a retarded company!
                        std::ranges::transform(wFilePath, outFilePath.begin(), [](wchar_t c)->char
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

//-------------------------------------------------------------------------------------------------
