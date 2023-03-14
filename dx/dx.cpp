/// -----------------------------------【头文件包含】-----------------------------------
// Windows 头文件
#include <windows.h>

/// -----------------------------------【宏定义部分】-----------------------------------
#define WINDOWTITLE L"致我们永不熄灭的游戏开发梦想~"
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

/// -----------------------------------【 函数声明 】----------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);


/// -----------------------------------【 入口函数 WinMain() 】----------------------------------
/// win窗口入口函数
int WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nCmdShow)
{
	// 注册窗口类
	WNDCLASSEX wndClass = { 0 };
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = WndProc;			//	消息处理函数回调	默认填 DefWindowProc 默认的消息处理 default Window procedure
	wndClass.hInstance = hInstance;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hIcon = nullptr;
	wndClass.hCursor = nullptr;
	wndClass.hbrBackground = nullptr;
	wndClass.lpszMenuName = nullptr;
	wndClass.lpszClassName = L"ForTheDreamOfGameDevelop";


	if (!RegisterClassEx(&wndClass))
	{
		MessageBox(0, L"注册类失败", 0, 0);
		return false;
	}

	// 创建窗口
	HWND hwnd = CreateWindowExW(
		WS_EX_OVERLAPPEDWINDOW,			// 扩展窗口样式
		L"ForTheDreamOfGameDevelop",	// 窗口类名
		WINDOWTITLE,					// 窗口标题
		WS_OVERLAPPEDWINDOW,			// 要创建的窗口类型
		CW_USEDEFAULT, CW_USEDEFAULT,	// 初始位置(x, y)（相对与父窗口）
		WINDOW_WIDTH, WINDOW_HEIGHT,	// 初始大小（宽度，长度）
		NULL,							// 此窗口的父级（父窗口句柄）
		NULL,							// 菜单栏句柄
		hInstance,						// 程序的句柄（WinMain 的第一个参数）
		NULL							// 附加数据。（一般不用）传递给窗口过程的值
	);

	if (!hwnd)
	{
		MessageBox(0, L"创建窗口失败。", 0, 0);
		return false;
	}

	// 窗口移动
	MoveWindow(hwnd, 250, 80, WINDOW_WIDTH, WINDOW_HEIGHT, true);
	// 显示窗口， nCmdShow显示的类型
	ShowWindow(hwnd, nCmdShow);
	// 更新窗口内容
	UpdateWindow(hwnd);

	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;

}

/// -----------------------------------【消息处理函数】----------------------------------
/// <summary>
///  消息处理函数 。（操作系统来调用）
/// </summary>
/// <param name="hwnd"></param>
/// <param name="message">消息类型</param>
/// <param name="wParam">消息内容</param>
/// <param name="lParam">消息内容</param>
/// <returns></returns>
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		break;
	}
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}