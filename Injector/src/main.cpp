typedef struct IUnknown IUnknown; // For XP Platform Toolset support

#include <windows.h>
#include <sstream>
#include <fstream>
#include <TlHelp32.h>

HWND WaitForWindow(const char* className, const char* windowTitle)
{
    // TODO: Try to replace by a HCBT_CREATEWND check, see https://stackoverflow.com/a/5188109
    HWND hWnd = nullptr;
    do
    {
        hWnd = FindWindowA(className, windowTitle);
        if (hWnd)
            return hWnd;
        Sleep(100);
    } while (true);
}

DWORD GetProcessIdByName(const wchar_t* processName)
{
    HANDLE hProcList = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (hProcList == INVALID_HANDLE_VALUE)
        return NULL;
    PROCESSENTRY32 procEntry;
    procEntry.dwSize = sizeof(procEntry);
    if (Process32First(hProcList, &procEntry))
    {
        while (Process32Next(hProcList, &procEntry))
        {
            if (!wcscmp(procEntry.szExeFile, processName))
            {
                DWORD dwPID = procEntry.th32ProcessID;
                CloseHandle(hProcList);
                return dwPID;
            }
        }
    }
    return NULL;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    std::string errorMsg;

    // File paths
    size_t pos;
    // injector
    char injectorPath[MAX_PATH];
    GetModuleFileNameA(NULL, injectorPath, MAX_PATH);
    // Dll extension
    std::string dllPath(injectorPath);
    pos = dllPath.find_last_of('\\');
    if (pos != std::string::npos)
        dllPath.replace(pos + 1, std::string::npos, "c1cx_lib.dll");
    // .ini
    std::string iniPath(injectorPath);
    pos = iniPath.find_last_of('\\');
    if (pos != std::string::npos)
        iniPath.replace(pos + 1, std::string::npos, "config.ini");
    // game
    std::string gamePath;
    // Initialize game path
    std::ifstream iniFile(iniPath);
    if (iniFile.good())
    {
        if (iniFile.is_open())
        {
            std::string line;
            while (std::getline(iniFile, line))
            {
                if (line.find("GamePath=") != std::string::npos)
                {
                    gamePath = line.substr(line.find("=") + 1);
                    break;
                }
            }
            iniFile.close();
        }
        else
        {
            errorMsg = "FAIL iniFile.is_open";
        }
    }
    else
    {
        // User selects CoDMP.exe
        int msgboxID = MessageBoxA(NULL, "Point me to your CoDMP file", "c1cx", MB_OKCANCEL | MB_ICONINFORMATION);
        if (msgboxID == IDCANCEL)
            return 0;
        OPENFILENAMEA ofn;
        char filename[MAX_PATH];
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = filename;
        ofn.lpstrFile[0] = '\0';
        ofn.nMaxFile = sizeof(filename);
        ofn.lpstrFilter = "Executable Files\0*.exe\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileNameA(&ofn))
        {
            gamePath = ofn.lpstrFile;
            std::ofstream iniFile(iniPath);
            if (iniFile.is_open())
            {
                iniFile << "GamePath=" << gamePath;
                iniFile.close();
            }
        }
        else
        {
            errorMsg = "FAIL GetOpenFileNameA";
        }
    }

    if (errorMsg.empty())
    {
        // Explicitly set current directory to prevent error when starting from .ini path
        size_t lastBackslashPos = gamePath.find_last_of("\\/");
        if (lastBackslashPos != std::string::npos)
        {
            std::string gameDir = gamePath.substr(0, lastBackslashPos);
            SetCurrentDirectoryA(gameDir.c_str());
        }

        size_t found = gamePath.find_last_of("\\");
        if (found != std::string::npos)
        {
            std::string gameFilename = gamePath.substr(found + 1);
            std::wstring gameFilenameW = std::wstring(gameFilename.begin(), gameFilename.end());

            DWORD gameProcessId = NULL;
            gameProcessId = GetProcessIdByName(gameFilenameW.c_str());
            if (gameProcessId == NULL)
            {
                //Game is not running, start it.
                STARTUPINFOA info = { sizeof(info) };
                PROCESS_INFORMATION processInfo;
                if (CreateProcessA(gamePath.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &info, &processInfo))
                {
                    CloseHandle(processInfo.hThread);
                    CloseHandle(processInfo.hProcess);
                    gameProcessId = processInfo.dwProcessId;
                }
                else
                {
                    errorMsg = "FAIL CreateProcessA";
                }
            }

            if (errorMsg.empty())
            {
                HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, gameProcessId);
                if (hProcess != NULL)
                {
                    LPVOID pDllPath = VirtualAllocEx(hProcess, NULL, dllPath.length() + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
                    if (pDllPath != NULL)
                    {
                        if (WriteProcessMemory(hProcess, pDllPath, dllPath.c_str(), dllPath.length() + 1, NULL))
                        {
                            HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, pDllPath, 0, NULL);
                            if (hThread != NULL)
                            {
                                WaitForSingleObject(hThread, INFINITE);
                                VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
                                CloseHandle(hThread);
                                CloseHandle(hProcess);

                                HWND hWnd = WaitForWindow(NULL, "Call of Duty Multiplayer");
                                std::string imguiIniPath(injectorPath);
                                pos = imguiIniPath.find_last_of('\\');
                                if (pos != std::string::npos)
                                    imguiIniPath.replace(pos + 1, std::string::npos, "imgui.ini");

                                COPYDATASTRUCT cds;
                                cds.dwData = 1; // Custom identifier for the data
                                cds.cbData = (DWORD)imguiIniPath.size() + 1;
                                cds.lpData = (PVOID)imguiIniPath.c_str();
                                SendMessageA(hWnd, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)(LPVOID)&cds);

                                return 0;
                            }
                            else
                            {
                                errorMsg = "FAIL CreateRemoteThread";
                            }
                        }
                        else
                        {
                            errorMsg = "FAIL WriteProcessMemory";
                            VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
                            CloseHandle(hProcess);
                        }
                    }
                    else
                    {
                        errorMsg = "FAIL VirtualAllocEx";
                    }
                }
                else
                {
                    errorMsg = "FAIL OpenProcess";
                }
            }
        }
    }

    if (!errorMsg.empty())
    {
        MessageBoxA(NULL, errorMsg.c_str(), "c1cx", MB_OK | MB_ICONERROR);
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}