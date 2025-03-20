// main.cpp
// Compile with: cl /EHsc main.cpp /link ws2_32.lib Shell32.lib

// Define WIN32_LEAN_AND_MEAN to prevent windows.h from including winsock.h
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <winsock2.h> // Must come before ws2tcpip.h
#include <ws2tcpip.h>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <codecvt> // For string conversions
                                                            
#pragma comment(lib, "ws2_32.lib")

// Global variables
std::string allowedIP = "0.0.0.0"; // Will be read from file
std::atomic<bool> g_Running(true);
const int HTTP_PORT = 8000;       // Change port if desired
const char* ALLOWED_IP_FILE = "allowed_ip.txt";

// Helper function to convert from std::string to std::wstring
std::wstring string_to_wstring(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}

// Forward declarations for the tray window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void AddTrayIcon(HWND hwnd);
void RemoveTrayIcon(HWND hwnd);
void ShowContextMenu(HWND hwnd, POINT pt);

//---------------------------------------------------------------------
// Read the allowed IP from a file
bool ReadAllowedIP(const char* filename, std::string& ip)
{
    std::ifstream file(filename);
    if (!file)
        return false;
    std::getline(file, ip);
    file.close();
    return !ip.empty();
}

//---------------------------------------------------------------------
// Execute a command using psshutdown.exe with given flags
void ExecutePowerCommand(const std::string& flags)
{
    std::string command = ".\\psshutdown.exe " + flags;
    // Execute the command (note: in a real-world app, consider using CreateProcess)
    system(command.c_str());
}

//---------------------------------------------------------------------
// Handle a single HTTP connection
void HandleClient(SOCKET clientSock)
{
    const int bufferSize = 1024;
    char buffer[bufferSize] = { 0 };
    int recvResult = recv(clientSock, buffer, bufferSize - 1, 0);
    if (recvResult <= 0)
    {
        closesocket(clientSock);
        return;
    }
    buffer[recvResult] = '\0';
    std::string request(buffer);

    // Get the remote IP address
    sockaddr_in clientAddr;
    int addrLen = sizeof(clientAddr);
    getpeername(clientSock, (sockaddr*)&clientAddr, &addrLen);
    char ipBuffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, ipBuffer, INET_ADDRSTRLEN);
    std::string clientIP(ipBuffer);

    // Check allowed IP
    if (clientIP != allowedIP)
    {
        std::string forbidden = "HTTP/1.1 403 Forbidden\r\nContent-Type: text/html\r\n\r\nForbidden: Your IP is not allowed.";
        send(clientSock, forbidden.c_str(), (int)forbidden.size(), 0);
        closesocket(clientSock);
        return;
    }

    // Determine the endpoint from the GET request line
    // Very simple parsing – look for "GET /endpoint"
    std::istringstream reqStream(request);
    std::string method, path, protocol;
    reqStream >> method >> path >> protocol;

    std::string response;
    std::string commandFlags;
    if (path == "/sleep")
    {
        response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\nSystem going to sleep...";
        commandFlags = "-d -t 0";
    }
    else if (path == "/shutdown")
    {
        response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\nSystem shutting down...";
        commandFlags = "-s -t 0";
    }
    else if (path == "/restart")
    {
        response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\nSystem restarting...";
        commandFlags = "-r -t 0";
    }
    else if (path == "/hibernate")
    {
        response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\nSystem hibernating...";
        commandFlags = "-h -t 0";
    }
    else
    {
        response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\nNot Found";
    }

    send(clientSock, response.c_str(), (int)response.size(), 0);
    closesocket(clientSock);

    // If a valid command is determined, execute it.
    if (!commandFlags.empty())
    {
        ExecutePowerCommand(commandFlags);
    }
}

//---------------------------------------------------------------------
// Simple HTTP server running on a separate thread.
void RunHttpServer()
{
    WSADATA wsaData;
    SOCKET listenSock = INVALID_SOCKET;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        return;
    }

    listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET)
    {
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(HTTP_PORT);

    if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        closesocket(listenSock);
        WSACleanup();
        return;
    }

    if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR)
    {
        closesocket(listenSock);
        WSACleanup();
        return;
    }

    while (g_Running)
    {
        sockaddr_in clientAddr;
        int addrLen = sizeof(clientAddr);
        SOCKET clientSock = accept(listenSock, (sockaddr*)&clientAddr, &addrLen);
        if (clientSock == INVALID_SOCKET)
        {
            break;
        }
        // Handle each client connection in a separate thread (or inline for simplicity)
        std::thread clientThread(HandleClient, clientSock);
        clientThread.detach();
    }

    closesocket(listenSock);
    WSACleanup();
}

//---------------------------------------------------------------------
// Window class name for the hidden window used for the tray icon
const wchar_t g_szClassName[] = L"TrayIconWindowClass";

//---------------------------------------------------------------------
// Entry point: WinMain creates a hidden window, adds a system tray icon,
// and launches the HTTP server thread.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    // Read the allowed IP from file
    if (!ReadAllowedIP(ALLOWED_IP_FILE, allowedIP))
    {
        MessageBoxW(NULL, L"Failed to read allowed IP from file.", L"Error", MB_ICONERROR);
        return 1;
    }

    // Start HTTP server thread
    std::thread httpThread(RunHttpServer);

    // Register window class for tray icon
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = g_szClassName;
    wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    if (!RegisterClassExW(&wc))
    {
        return 1;
    }

    // Create hidden window
    HWND hwnd = CreateWindowExW(0, g_szClassName, L"Tray Icon Window", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 200, NULL, NULL, hInstance, NULL);
    if (!hwnd)
    {
        return 1;
    }

    // Hide the window
    ShowWindow(hwnd, SW_HIDE);

    // Add system tray icon
    AddTrayIcon(hwnd);

    // Message loop for tray icon
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Clean up
    g_Running = false; // Signal HTTP server to stop
    // Optionally, wait for the httpThread to finish (or force termination)
    httpThread.join();
    RemoveTrayIcon(hwnd);
    return (int)msg.wParam;
}

//---------------------------------------------------------------------
// Add an icon to the system tray.
void AddTrayIcon(HWND hwnd)
{
    NOTIFYICONDATAW nid = { 0 };
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1; // Identifier for the icon
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER + 1; // Custom message for tray icon events
    nid.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    lstrcpyW(nid.szTip, L"Power Control Daemon");
    Shell_NotifyIconW(NIM_ADD, &nid);
}

//---------------------------------------------------------------------
// Remove the system tray icon.
void RemoveTrayIcon(HWND hwnd)
{
    NOTIFYICONDATAW nid = { 0 };
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    Shell_NotifyIconW(NIM_DELETE, &nid);
}

//---------------------------------------------------------------------
// Show a simple context menu when the tray icon is right-clicked.
void ShowContextMenu(HWND hwnd, POINT pt)
{
    HMENU hMenu = CreatePopupMenu();
    if (hMenu)
    {
        // Add an "Exit" menu item
        AppendMenuW(hMenu, MF_STRING, WM_APP + 1, L"Exit");

        // Set foreground window and track the popup
        SetForegroundWindow(hwnd);
        TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
        DestroyMenu(hMenu);
    }
}

//---------------------------------------------------------------------
// Window procedure for handling messages for our hidden window.
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_USER + 1: // Tray icon callback message
        if (LOWORD(lParam) == WM_RBUTTONUP)
        {
            POINT pt;
            GetCursorPos(&pt);
            ShowContextMenu(hwnd, pt);
        }
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == WM_APP + 1) // Exit menu item selected
        {
            PostQuitMessage(0);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
