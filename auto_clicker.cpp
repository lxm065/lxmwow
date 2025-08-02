#include <windows.h>
#include <string>
#include <ctime>
#include <thread>
#include <sstream>

// Global variables
bool clicking = false;
int targetX1 = 0, targetY1 = 0;  // 第一个点击点
int targetX2 = 0, targetY2 = 0;  // 第二个点击点
int intervalMs = 300;           // 默认间隔时间为300毫秒
HWND hwndLog, hwndInterval;     // 日志窗口和间隔时间输入框
std::thread clickThread;
HWND g_hwndMain = NULL; // 主窗口句柄

// 自定义消息用于日志更新
#define WM_UPDATE_LOG (WM_USER + 100)

// Log click event
void LogClick(int x, int y, const char* pointName) {
    time_t now = time(nullptr);
    char timeStr[26];
    ctime_s(timeStr, sizeof(timeStr), &now);
    timeStr[24] = '\0';
    std::string log = std::string(timeStr) + " - " + pointName + 
                      " (" + std::to_string(x) + ", " + std::to_string(y) + ")\n";
    
    // 通过消息机制在主线程更新日志
    if (g_hwndMain) {
        // 分配内存并复制日志字符串
        char* logText = new char[log.size() + 1];
        strcpy_s(logText, log.size() + 1, log.c_str());
        PostMessage(g_hwndMain, WM_UPDATE_LOG, (WPARAM)logText, 0);
    }
}

// 不移动鼠标的模拟点击
void SimulateClickWithoutMove(int x, int y) {
    // 获取目标位置的屏幕坐标
    POINT originalPos;
    GetCursorPos(&originalPos); // 保存原始鼠标位置
    
    // 设置鼠标位置到目标点（不移动物理鼠标）
    SetCursorPos(x, y);
    
    // 创建鼠标事件
    INPUT inputs[2] = {};
    
    // 鼠标按下事件
    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    
    // 鼠标释放事件
    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    
    // 发送鼠标事件
    SendInput(2, inputs, sizeof(INPUT));
    
    // 立即将鼠标移回原始位置
    SetCursorPos(originalPos.x, originalPos.y);
}

// 只移动鼠标不点击
void MoveCursorWithoutClick(int x, int y) {
    // 获取目标位置的屏幕坐标
    POINT originalPos;
    GetCursorPos(&originalPos); // 保存原始鼠标位置
    
    // 设置鼠标位置到目标点
    SetCursorPos(x, y);
    
    // 立即将鼠标移回原始位置
    SetCursorPos(originalPos.x, originalPos.y);
}

// Click loop
void ClickLoop() {
    // 记录开始信息
    std::string initMsg = "Starting auto clicker with " + std::to_string(intervalMs) + "ms interval\n"
                         "Point 1: (" + std::to_string(targetX1) + ", " + std::to_string(targetY1) + ")\n" +
                         "Point 2: (" + std::to_string(targetX2) + ", " + std::to_string(targetY2) + ")\n";
    char* initText = new char[initMsg.size() + 1];
    strcpy_s(initText, initMsg.size() + 1, initMsg.c_str());
    PostMessage(g_hwndMain, WM_UPDATE_LOG, (WPARAM)initText, 0);
    
    while (clicking) {
        // 点击第一个点
        SimulateClickWithoutMove(targetX1, targetY1);
        LogClick(targetX1, targetY1, "Clicked Point 1");
        
        // 等待指定间隔
        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
        
        if (!clicking) break;  // 检查是否在等待期间停止了
        
        // 仅移动到第二个点，不点击
        MoveCursorWithoutClick(targetX2, targetY2);
        LogClick(targetX2, targetY2, "Moved to Point 2");
        
        // 等待指定间隔
        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
    }
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_UPDATE_LOG: {
        // 安全地在主线程更新日志
        char* logText = (char*)wParam;
        if (logText && hwndLog) {
            // 获取当前文本长度
            int len = GetWindowTextLengthA(hwndLog);
            // 设置插入位置到末尾
            SendMessageA(hwndLog, EM_SETSEL, len, len);
            // 替换选中文本（即追加）
            SendMessageA(hwndLog, EM_REPLACESEL, 0, (LPARAM)logText);
            // 滚动到可见位置
            SendMessageA(hwndLog, EM_SCROLLCARET, 0, 0);
        }
        delete[] logText; // 释放内存
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) { // Start button
            if (!clicking) {
                // 获取间隔时间
                char buffer[32];
                GetWindowTextA(hwndInterval, buffer, sizeof(buffer));
                intervalMs = std::atoi(buffer);
                
                // 验证间隔时间
                if (intervalMs < 10) {
                    intervalMs = 100; // 最小值100ms
                } else if (intervalMs > 60000) {
                    intervalMs = 60000; // 最大值60秒
                }
                
                POINT p;
                // 获取当前鼠标位置作为第一个点
                GetCursorPos(&p);
                targetX1 = p.x;
                targetY1 = p.y;
                
                // 第二个点固定为第一个点右侧100像素
                targetX2 = targetX1 + 100;
                targetY2 = targetY1;
                
                clicking = true;
                
                // 启动点击线程
                clickThread = std::thread(ClickLoop);
                clickThread.detach();
            }
        }
        if (LOWORD(wParam) == 2) { // Stop button
            if (clicking) {
                clicking = false;
                std::string log = "Stopped clicking\n";
                char* logText = new char[log.size() + 1];
                strcpy_s(logText, log.size() + 1, log.c_str());
                PostMessage(g_hwndMain, WM_UPDATE_LOG, (WPARAM)logText, 0);
                
                // 确保线程结束
                if (clickThread.joinable()) {
                    clickThread.join();
                }
            }
        }
        break;
    case WM_DESTROY:
        clicking = false; // 确保停止点击线程
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Hotkey handling
void HandleHotkeys() {
    while (true) {
        if (GetAsyncKeyState(VK_F1) & 0x8000) {
            // 模拟按钮点击
            PostMessageA(g_hwndMain, WM_COMMAND, 1, 0);
            Sleep(200); // 防止重复触发
        }
        if (GetAsyncKeyState(VK_F2) & 0x8000) {
            PostMessageA(g_hwndMain, WM_COMMAND, 2, 0);
            Sleep(200);
        }
        Sleep(10);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Register window class
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "AutoClickerClass";
    RegisterClassA(&wc);

    // Create window
    HWND hwnd = CreateWindowA("AutoClickerClass", "Auto Clicker", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 450, NULL, NULL, hInstance, NULL);
    g_hwndMain = hwnd; // 保存主窗口句柄

    // Create controls
    CreateWindowA("STATIC", "Auto Clicker - Custom Interval", WS_VISIBLE | WS_CHILD,
        10, 10, 300, 20, hwnd, NULL, hInstance, NULL);
    
    // 间隔时间输入框
    CreateWindowA("STATIC", "Click Interval (ms):", WS_VISIBLE | WS_CHILD,
        10, 40, 120, 20, hwnd, NULL, hInstance, NULL);
    hwndInterval = CreateWindowA("EDIT", "300", WS_VISIBLE | WS_CHILD | WS_BORDER,
        130, 40, 100, 20, hwnd, NULL, hInstance, NULL);
    
    // 按钮
    CreateWindowA("BUTTON", "Set Point & Start (F1)", WS_VISIBLE | WS_CHILD,
        10, 70, 150, 30, hwnd, (HMENU)1, hInstance, NULL);
    CreateWindowA("BUTTON", "Stop (F2)", WS_VISIBLE | WS_CHILD,
        170, 70, 150, 30, hwnd, (HMENU)2, hInstance, NULL);
    
    // 日志框
    hwndLog = CreateWindowA("EDIT", "Auto Clicker Log:\n", 
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL,
        10, 110, 460, 300, hwnd, NULL, hInstance, NULL);

    // 设置等宽字体
    HFONT hFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                           DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");
    SendMessage(hwndLog, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hwndInterval, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // 设置文本限制为0（无限制）
    SendMessage(hwndLog, EM_SETLIMITTEXT, 0, 0);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Start hotkey thread
    std::thread hotkeyThread(HandleHotkeys);
    hotkeyThread.detach();

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}