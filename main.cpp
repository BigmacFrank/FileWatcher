#include <windows.h>
#include <iostream>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

constexpr DWORD BUFFER_SIZE = 4096;
const std::wstring WATCH_PATH = L"C:\\";

// 🕒 Get current timestamp in HH:MM:SS format
std::wstring GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm{};
    localtime_s(&local_tm, &now_c);

    std::wstringstream ss;
    ss << std::put_time(&local_tm, L"%H:%M:%S");
    return ss.str();
}

// 📁 Check if a path is a directory
bool IsDirectory(const std::wstring& path) {
    DWORD attr = GetFileAttributesW(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

int main() {
    HANDLE dirHandle = CreateFileW(
        WATCH_PATH.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr
    );

    if (dirHandle == INVALID_HANDLE_VALUE) {
        std::wcerr << L"ERROR: Could not open directory. Code: " << GetLastError() << std::endl;
        return 1;
    }

    std::wcout << L"[FileChecker Active] Watching: " << WATCH_PATH << std::endl;

    char buffer[BUFFER_SIZE];
    DWORD bytesReturned;

    while (true) {
        BOOL success = ReadDirectoryChangesW(
            dirHandle,
            &buffer,
            sizeof(buffer),
            TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME |
            FILE_NOTIFY_CHANGE_DIR_NAME |
            FILE_NOTIFY_CHANGE_ATTRIBUTES |
            FILE_NOTIFY_CHANGE_SIZE,
            &bytesReturned,
            nullptr,
            nullptr
        );

        if (!success) break;

        FILE_NOTIFY_INFORMATION* info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);
        do {
            std::wstring name(info->FileName, info->FileNameLength / sizeof(WCHAR));
            std::wstring fullPath = WATCH_PATH + L"\\" + name;
            std::wstring timestamp = GetTimestamp();
            std::wstring log;

            switch (info->Action) {
                case FILE_ACTION_ADDED:
                    log = IsDirectory(fullPath)
                        ? L"(Folder Creation): New folder in \"" + WATCH_PATH + L"\" New Folder is \"" + name + L"\" " + timestamp
                        : L"(File Creation): \"" + WATCH_PATH + L"\" New File is \"" + name + L"\" " + timestamp;
                    break;

                case FILE_ACTION_REMOVED:
                    log = IsDirectory(fullPath)
                        ? L"(Folder Deletion): \"" + WATCH_PATH + L"\" Deleted Folder was \"" + name + L"\" " + timestamp
                        : L"(File Deletion): \"" + WATCH_PATH + L"\" Deleted File was \"" + name + L"\" " + timestamp;
                    break;

                case FILE_ACTION_MODIFIED:
                    log = L"(Property Change): \"" + WATCH_PATH + L"\" File \"" + name + L"\" changed " + timestamp;
                    break;

                case FILE_ACTION_RENAMED_OLD_NAME:
                    log = L"(Renamed): \"" + WATCH_PATH + L"\" Old Name was \"" + name + L"\" " + timestamp;
                    break;

                case FILE_ACTION_RENAMED_NEW_NAME:
                    log = L"(Renamed): \"" + WATCH_PATH + L"\" New Name is \"" + name + L"\" " + timestamp;
                    break;

                default:
                    log = L"(Unknown Event): \"" + WATCH_PATH + L"\" Item \"" + name + L"\" " + timestamp;
                    break;
            }

            std::wcout << log << std::endl;

            if (info->NextEntryOffset == 0) break;
            info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                reinterpret_cast<BYTE*>(info) + info->NextEntryOffset
            );
        } while (true);
    }

    CloseHandle(dirHandle);
    return 0;
}