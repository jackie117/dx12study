/// -----------------------------------��ͷ�ļ�������-----------------------------------
// Windows ͷ�ļ�
#include <windows.h>
/// -----------------------------------------------------------------------------------

/// -----------------------------------���궨�岿�֡�-----------------------------------
#define WINDOWTITLE L"����������Ϩ�����Ϸ��������~"
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
/// -----------------------------------------------------------------------------------

/// -----------------------------------��ȫ�ֱ�����-----------------------------------
float g_posX = 0;
float g_posY = 0;
HINSTANCE g_hInstance = 0;
int g_nCmdShow = 0;
HWND g_hwnd = 0;
/// ---------------------------------------------------------------------------------

/// -----------------------------------�� �������� ��----------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void InitWindow();	//��ʼ������
void Init();		//��ʼ������
void UpdateScene();	//���»��Ƶ�����
void DrawScene();	//���л��Ʋ���
int Run();			//����
/// ---------------------------------------------------------------------------------

/// -----------------------------------�� ��ں��� WinMain() ��----------------------------------
/// win������ں���
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
	// ע�ᴰ����
	WNDCLASSEX wndClass = { 0 };
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = WndProc;			//	��Ϣ�������ص�	Ĭ���� DefWindowProc Ĭ�ϵ���Ϣ���� default Window procedure
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
		MessageBox(0, L"ע����ʧ��", 0, 0);
		return;
	}

	// ��������
	HWND hwnd = CreateWindowExW(
		WS_EX_OVERLAPPEDWINDOW,			// ��չ������ʽ
		L"ForTheDreamOfGameDevelop",	// ��������
		WINDOWTITLE,					// ���ڱ���
		WS_OVERLAPPEDWINDOW,			// Ҫ�����Ĵ�������
		CW_USEDEFAULT, CW_USEDEFAULT,	// ��ʼλ��(x, y)������븸���ڣ�
		WINDOW_WIDTH, WINDOW_HEIGHT,	// ��ʼ��С����ȣ����ȣ�
		NULL,							// �˴��ڵĸ����������ھ����
		NULL,							// �˵������
		g_hInstance,					// ����ľ����WinMain �ĵ�һ��������
		NULL							// �������ݡ���һ�㲻�ã����ݸ����ڹ��̵�ֵ
	);

	if (!hwnd)
	{
		MessageBox(0, L"��������ʧ�ܡ�", 0, 0);
		return;
	}

	// �����ƶ���
	MoveWindow(hwnd, 250, 80, WINDOW_WIDTH, WINDOW_HEIGHT, true);
	// ��ʾ���ڣ� g_nCmdShow��ʾ������
	ShowWindow(hwnd, g_nCmdShow);
	// ���´�������
	UpdateWindow(hwnd);
}

/// -----------------------------------����Ϣ��������----------------------------------
/// <summary>
///  ��Ϣ������ ��������ϵͳ�����ã�
/// </summary>
/// <param name="hwnd"></param>
/// <param name="message">��Ϣ����</param>
/// <param name="wParam">��Ϣ����</param>
/// <param name="lParam">��Ϣ����</param>
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

/// -----------------------------------�����л��ơ�-----------------------------------
/// ����������Ϣ���л���
void DrawScene()
{
	HDC hdc = GetDC(g_hwnd);
	TextOut(hdc, g_posX, g_posY, L"hello", 5);
	ReleaseDC(g_hwnd, hdc);
}
/// ---------------------------------------------------------------------------------

//-----------------------------------�����»��Ƶ����ݡ�-----------------------------------
// ����������Ϣ
void UpdateScene()
{
	g_posY += 0.001f;
}
/// ---------------------------------------------------------------------------------

/// -----------------------------------����Ϸ���к�����-----------------------------------
/// ��Ϸѭ����ܷ�װ������
/// ����Ϣѭ��ѡ��˭����
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