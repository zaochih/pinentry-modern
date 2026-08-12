#include "pch.h"
#include "OwnerIcon.h"

#include <cstddef>
#include <shellapi.h>
#include <tlhelp32.h>
#include <wincodec.h>
#include <winver.h>

namespace PinentryWinUI3::Platform
{
    namespace
    {
        bool OwnerTraceEnabled()
        {
            wchar_t value[16] = {};
            DWORD const length = GetEnvironmentVariableW(
                L"PINENTRY_WINUI3_OWNER_TRACE",
                value,
                static_cast<DWORD>(sizeof(value) / sizeof(value[0])));
            return length == 0 || wcscmp(value, L"0") != 0;
        }

        std::wstring OwnerTracePath()
        {
            wchar_t configuredPath[32768] = {};
            DWORD const configuredLength = GetEnvironmentVariableW(
                L"PINENTRY_WINUI3_OWNER_TRACE_LOG",
                configuredPath,
                static_cast<DWORD>(sizeof(configuredPath) / sizeof(configuredPath[0])));
            if (configuredLength > 0 && configuredLength < sizeof(configuredPath) / sizeof(configuredPath[0]))
            {
                return configuredPath;
            }

            std::wstring tempPath(MAX_PATH + 1, L'\0');
            DWORD const length = GetTempPathW(static_cast<DWORD>(tempPath.size()), tempPath.data());
            if (length == 0 || length >= tempPath.size())
            {
                return {};
            }

            tempPath.resize(length);
            return tempPath + L"pinentry-winui3-owner-trace.log";
        }

        void TraceOwner(std::wstring const& message)
        {
            if (!OwnerTraceEnabled())
            {
                return;
            }

            auto line = L"[pinentry-winui3 owner] " + message + L"\n";
            OutputDebugStringW(line.c_str());

            auto const path = OwnerTracePath();
            if (path.empty())
            {
                return;
            }

            std::wofstream log(path, std::ios::app);
            if (log)
            {
                log << line;
            }
        }

        std::wstring LastErrorMessage(DWORD error = GetLastError())
        {
            return L"error=" + std::to_wstring(error);
        }

        struct HandleCloser
        {
            void operator()(HANDLE handle) const noexcept
            {
                if (handle && handle != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(handle);
                }
            }
        };

        using UniqueHandle = std::unique_ptr<std::remove_pointer_t<HANDLE>, HandleCloser>;

        struct IconCloser
        {
            void operator()(HICON icon) const noexcept
            {
                if (icon)
                {
                    DestroyIcon(icon);
                }
            }
        };

        using UniqueIcon = std::unique_ptr<std::remove_pointer_t<HICON>, IconCloser>;

        std::wstring FileNameFromPath(std::wstring const& path)
        {
            auto const separator = path.find_last_of(L"\\/");
            if (separator == std::wstring::npos)
            {
                return path;
            }

            return path.substr(separator + 1);
        }

        std::wstring FileStemFromPath(std::wstring const& path)
        {
            auto fileName = FileNameFromPath(path);
            auto const dot = fileName.find_last_of(L'.');
            if (dot == std::wstring::npos || dot == 0)
            {
                return fileName;
            }

            return fileName.substr(0, dot);
        }

        unsigned long CopyParentProcessID(unsigned long processID)
        {
            UniqueHandle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
            if (!snapshot || snapshot.get() == INVALID_HANDLE_VALUE)
            {
                TraceOwner(L"CreateToolhelp32Snapshot failed (" + LastErrorMessage() + L")");
                return 0;
            }

            PROCESSENTRY32W entry{};
            entry.dwSize = sizeof(entry);
            if (!Process32FirstW(snapshot.get(), &entry))
            {
                TraceOwner(L"Process32FirstW failed (" + LastErrorMessage() + L")");
                return 0;
            }

            do
            {
                if (entry.th32ProcessID == processID)
                {
                    return entry.th32ParentProcessID;
                }
            } while (Process32NextW(snapshot.get(), &entry));

            TraceOwner(L"parent pid not found for pid " + std::to_wstring(processID));
            return 0;
        }

        std::wstring Lowercase(std::wstring value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch)
            {
                return static_cast<wchar_t>(towlower(ch));
            });
            return value;
        }

        bool IsPinentryInfrastructureProcess(std::wstring const& imagePath)
        {
            auto const fileName = Lowercase(FileNameFromPath(imagePath));
            return
                fileName == L"pinentry-winui3.exe" ||
                fileName == L"pinentry.exe" ||
                fileName == L"gpg-agent.exe";
        }

        std::wstring QueryVersionString(
            std::vector<std::byte> const& versionData,
            WORD language,
            WORD codePage,
            wchar_t const* key)
        {
            wchar_t subBlock[96] = {};
            swprintf_s(
                subBlock,
                L"\\StringFileInfo\\%04x%04x\\%s",
                language,
                codePage,
                key);

            LPVOID value = nullptr;
            UINT valueLength = 0;
            if (!VerQueryValueW(
                    versionData.data(),
                    subBlock,
                    &value,
                    &valueLength) ||
                !value ||
                valueLength == 0)
            {
                return {};
            }

            return std::wstring(static_cast<wchar_t*>(value));
        }

        std::wstring ResolveFileDisplayName(std::wstring const& path)
        {
            DWORD handle = 0;
            DWORD const versionSize = GetFileVersionInfoSizeW(path.c_str(), &handle);
            if (versionSize == 0)
            {
                TraceOwner(L"GetFileVersionInfoSizeW failed for " + path + L" (" + LastErrorMessage() + L")");
                return FileStemFromPath(path);
            }

            std::vector<std::byte> versionData(versionSize);
            if (!GetFileVersionInfoW(path.c_str(), 0, versionSize, versionData.data()))
            {
                TraceOwner(L"GetFileVersionInfoW failed for " + path + L" (" + LastErrorMessage() + L")");
                return FileStemFromPath(path);
            }

            struct Translation
            {
                WORD language;
                WORD codePage;
            };

            LPVOID translationsRaw = nullptr;
            UINT translationsBytes = 0;
            std::vector<Translation> translations;
            if (VerQueryValueW(
                    versionData.data(),
                    L"\\VarFileInfo\\Translation",
                    &translationsRaw,
                    &translationsBytes) &&
                translationsRaw &&
                translationsBytes >= sizeof(Translation))
            {
                auto const count = translationsBytes / sizeof(Translation);
                auto const* translation = static_cast<Translation const*>(translationsRaw);
                translations.assign(translation, translation + count);
            }

            translations.push_back({ 0x0409, 1200 });
            translations.push_back({ 0x0409, 1252 });
            translations.push_back({ 0x0000, 1200 });

            for (auto const& translation : translations)
            {
                wchar_t const* keys[] = { L"FileDescription", L"ProductName" };
                for (auto const* key : keys)
                {
                    auto value = QueryVersionString(
                        versionData,
                        translation.language,
                        translation.codePage,
                        key);
                    if (!value.empty())
                    {
                        TraceOwner(L"resolved display name: " + value);
                        return value;
                    }
                }
            }

            auto const fallback = FileStemFromPath(path);
            TraceOwner(L"using file-stem display name: " + fallback);
            return fallback;
        }

        bool SaveIconAsPng(HICON icon, std::wstring const& path)
        {
            if (!icon)
            {
                return false;
            }

            winrt::com_ptr<IWICImagingFactory> factory;
            HRESULT result = CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(factory.put()));
            if (FAILED(result))
            {
                TraceOwner(L"CoCreateInstance(CLSID_WICImagingFactory) failed: hr=" + std::to_wstring(result));
                return false;
            }

            winrt::com_ptr<IWICBitmap> bitmap;
            result = factory->CreateBitmapFromHICON(icon, bitmap.put());
            if (FAILED(result))
            {
                TraceOwner(L"CreateBitmapFromHICON failed: hr=" + std::to_wstring(result));
                return false;
            }

            winrt::com_ptr<IWICStream> stream;
            result = factory->CreateStream(stream.put());
            if (FAILED(result))
            {
                TraceOwner(L"CreateStream failed: hr=" + std::to_wstring(result));
                return false;
            }

            result = stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE);
            if (FAILED(result))
            {
                TraceOwner(L"InitializeFromFilename failed for " + path + L": hr=" + std::to_wstring(result));
                return false;
            }

            winrt::com_ptr<IWICBitmapEncoder> encoder;
            result = factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, encoder.put());
            if (FAILED(result))
            {
                TraceOwner(L"CreateEncoder(PNG) failed: hr=" + std::to_wstring(result));
                return false;
            }

            result = encoder->Initialize(stream.get(), WICBitmapEncoderNoCache);
            if (FAILED(result))
            {
                TraceOwner(L"encoder Initialize failed: hr=" + std::to_wstring(result));
                return false;
            }

            winrt::com_ptr<IWICBitmapFrameEncode> frame;
            result = encoder->CreateNewFrame(frame.put(), nullptr);
            if (FAILED(result))
            {
                TraceOwner(L"CreateNewFrame failed: hr=" + std::to_wstring(result));
                return false;
            }

            result = frame->Initialize(nullptr);
            if (FAILED(result))
            {
                TraceOwner(L"frame Initialize failed: hr=" + std::to_wstring(result));
                return false;
            }

            result = frame->WriteSource(bitmap.get(), nullptr);
            if (FAILED(result))
            {
                TraceOwner(L"WriteSource failed: hr=" + std::to_wstring(result));
                return false;
            }

            result = frame->Commit();
            if (FAILED(result))
            {
                TraceOwner(L"frame Commit failed: hr=" + std::to_wstring(result));
                return false;
            }

            result = encoder->Commit();
            if (FAILED(result))
            {
                TraceOwner(L"encoder Commit failed: hr=" + std::to_wstring(result));
                return false;
            }

            TraceOwner(L"wrote owner icon png: " + path);
            return true;
        }

        std::wstring TemporaryIconPath(unsigned long processID)
        {
            std::wstring tempPath(MAX_PATH + 1, L'\0');
            DWORD const length = GetTempPathW(static_cast<DWORD>(tempPath.size()), tempPath.data());
            if (length == 0 || length >= tempPath.size())
            {
                TraceOwner(L"GetTempPathW failed (" + LastErrorMessage() + L")");
                return {};
            }

            tempPath.resize(length);
            return tempPath +
                L"pinentry-winui3-owner-" +
                std::to_wstring(processID) +
                L"-" +
                std::to_wstring(GetTickCount64()) +
                L".png";
        }

        std::wstring ExtractProcessIconToPng(std::wstring const& imagePath, unsigned long processID)
        {
            HICON largeIcon = nullptr;
            HICON smallIcon = nullptr;
            if (ExtractIconExW(imagePath.c_str(), 0, &largeIcon, &smallIcon, 1) == 0)
            {
                TraceOwner(L"ExtractIconExW failed for " + imagePath + L" (" + LastErrorMessage() + L")");
                return {};
            }

            UniqueIcon largeIconHandle(largeIcon);
            UniqueIcon smallIconHandle(smallIcon);
            HICON icon = largeIconHandle ? largeIconHandle.get() : smallIconHandle.get();
            if (!icon)
            {
                TraceOwner(L"ExtractIconExW returned no usable icon for " + imagePath);
                return {};
            }

            auto const iconPath = TemporaryIconPath(processID);
            if (iconPath.empty())
            {
                TraceOwner(L"temporary owner icon path is empty");
                return {};
            }

            if (!SaveIconAsPng(icon, iconPath))
            {
                DeleteFileW(iconPath.c_str());
                TraceOwner(L"failed to save owner icon png, deleted " + iconPath);
                return {};
            }

            return iconPath;
        }

        unsigned long ResolveFallbackOwnerProcessID()
        {
            unsigned long currentPID = GetCurrentProcessId();
            unsigned long agentPID = 0;
            std::vector<unsigned long> visited;

            for (int depth = 0; currentPID > 1 && depth < 12; ++depth)
            {
                if (std::find(visited.begin(), visited.end(), currentPID) != visited.end())
                {
                    TraceOwner(L"fallback parent walk detected a cycle at pid " + std::to_wstring(currentPID));
                    break;
                }
                visited.push_back(currentPID);

                auto imagePath = ResolveProcessImagePath(currentPID);
                TraceOwner(
                    L"fallback candidate depth=" +
                    std::to_wstring(depth) +
                    L", pid=" +
                    std::to_wstring(currentPID) +
                    L", image='" +
                    imagePath +
                    L"'");

                if (!imagePath.empty() && !IsPinentryInfrastructureProcess(imagePath))
                {
                    TraceOwner(L"using fallback owner pid: " + std::to_wstring(currentPID));
                    return currentPID;
                }
                if (agentPID == 0 && Lowercase(FileNameFromPath(imagePath)) == L"gpg-agent.exe")
                {
                    agentPID = currentPID;
                }

                auto const parentPID = CopyParentProcessID(currentPID);
                if (parentPID <= 1 || parentPID == currentPID)
                {
                    TraceOwner(L"fallback parent walk stopped at pid " + std::to_wstring(currentPID));
                    break;
                }

                currentPID = parentPID;
            }

            if (agentPID > 1)
            {
                TraceOwner(L"using gpg-agent as fallback owner pid: " + std::to_wstring(agentPID));
                return agentPID;
            }

            TraceOwner(L"fallback owner pid not found");
            return 0;
        }
    }

    std::wstring ResolveProcessImagePath(unsigned long processID)
    {
        if (processID <= 1)
        {
            TraceOwner(L"owner pid is not usable: " + std::to_wstring(processID));
            return {};
        }

        TraceOwner(L"resolving owner pid: " + std::to_wstring(processID));
        UniqueHandle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(processID)));
        if (!process)
        {
            TraceOwner(L"OpenProcess failed for pid " + std::to_wstring(processID) + L" (" + LastErrorMessage() + L")");
            return {};
        }

        std::wstring path(32768, L'\0');
        DWORD length = static_cast<DWORD>(path.size());
        if (!QueryFullProcessImageNameW(process.get(), 0, path.data(), &length))
        {
            TraceOwner(L"QueryFullProcessImageNameW failed for pid " + std::to_wstring(processID) + L" (" + LastErrorMessage() + L")");
            return {};
        }

        path.resize(length);
        TraceOwner(L"resolved process image path: " + path);
        return path;
    }

    OwnerApplicationInfo ResolveOwnerApplicationInfo(unsigned long processID)
    {
        OwnerApplicationInfo info;
        if (processID <= 1)
        {
            processID = ResolveFallbackOwnerProcessID();
        }

        info.imagePath = ResolveProcessImagePath(processID);
        if (info.imagePath.empty())
        {
            TraceOwner(L"owner application info has no image path");
            return info;
        }

        info.displayName = ResolveFileDisplayName(info.imagePath);
        info.iconPath = ExtractProcessIconToPng(info.imagePath, processID);
        TraceOwner(
            L"owner application info: pid=" +
            std::to_wstring(processID) +
            L", name='" +
            info.displayName +
            L"', icon='" +
            info.iconPath +
            L"'");
        return info;
    }
}
