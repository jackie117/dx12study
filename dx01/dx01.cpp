/// -----------------------------------【头文件包含】-----------------------------------
// Windows 头文件
#include <windows.h>
/// -----------------------------------------------------------------------------------

/// -----------------------------------【宏定义部分】-----------------------------------
#define WINDOWTITLE L"致我们永不熄灭的游戏开发梦想~"
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
/// -----------------------------------------------------------------------------------

/// -----------------------------------【全局变量】-----------------------------------
float g_posX = 0;
float g_posY = 0;
HINSTANCE g_hInstance = 0;
int g_nCmdShow = 0;
HWND g_hwnd = 0;
/// ---------------------------------------------------------------------------------

/// -----------------------------------【 函数声明 】----------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void InitWindow();	//初始化窗口
void Init();		//初始化程序
void UpdateScene();	//更新绘制的数据
void DrawScene();	//进行绘制操作
int Run();			//运行
/// ---------------------------------------------------------------------------------

/// -----------------------------------【 入口函数 WinMain() 】----------------------------------
/// win窗口入口函数
int WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nCmdShow)
{
	g_hInstance = hInstance;
	g_nCmdShow = nCmdShow;

	Init();

	int r = Run();
	return r;

}
/// ---------------------------------------------------------------------------------

void Init() 
{
	InitWindow();

	g_posX = 400.0f;
	g_posY = 1.0f;
}

void InitWindow()
{
	// 注册窗口类
	WNDCLASSEX wndClass = { 0 };
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = WndProc;			//	消息处理函数回调	默认填 DefWindowProc 默认的消息处理 default Window procedure
	wndClass.hInstance = g_hInstance;
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
		return;
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
		g_hInstance,					// 程序的句柄（WinMain 的第一个参数）
		NULL							// 附加数据。（一般不用）传递给窗口过程的值
	);

	if (!hwnd)
	{
		MessageBox(0, L"创建窗口失败。", 0, 0);
		return;
	}

	// 窗口移动，
	MoveWindow(hwnd, 250, 80, WINDOW_WIDTH, WINDOW_HEIGHT, true);
	// 显示窗口， g_nCmdShow显示的类型
	ShowWindow(hwnd, g_nCmdShow);
	// 更新窗口内容
	UpdateWindow(hwnd);
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
		{
			DestroyWindow(hwnd);
		}
		else if (wParam == VK_LEFT)
		{
			g_posX -= 1.0f;
		}
		else if (wParam == VK_RIGHT)
		{
			g_posX += 1.0f;
		}
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

/// -----------------------------------【进行绘制】-----------------------------------
/// 处理外来信息进行绘制
void DrawScene()
{
	HDC hdc = GetDC(g_hwnd);
	TextOut(hdc, g_posX, g_posY, L"hello", 5);
	ReleaseDC(g_hwnd, hdc);
}
/// ---------------------------------------------------------------------------------

//-----------------------------------【更新绘制的数据】-----------------------------------
// 处理外来信息
void UpdateScene()
{
	g_posY += 0.001f;
}
/// ---------------------------------------------------------------------------------

/// -----------------------------------【游戏运行函数】-----------------------------------
/// 游戏循环框架封装在其中
/// 即消息循环选择谁处理
int Run()
{
	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			UpdateScene();
			DrawScene();
		}
	}
	return (int)msg.wParam;
}
/// ---------------------------------------------------------------------------------