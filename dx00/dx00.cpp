/// -----------------------------------��ͷ�ļ�������-----------------------------------
// Windows ͷ�ļ�
#include <windows.h>

/// -----------------------------------���궨�岿�֡�-----------------------------------
#define WINDOWTITLE L"����������Ϩ�����Ϸ��������~"
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

/// -----------------------------------�� �������� ��----------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);


/// -----------------------------------�� ��ں��� WinMain() ��----------------------------------
/// win������ں���
int WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nCmdShow)
{
	// ע�ᴰ����
	WNDCLASSEX wndClass = { 0 };
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = WndProc;			//	��Ϣ�������ص�	Ĭ���� DefWindowProc Ĭ�ϵ���Ϣ���� default Window procedure
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
		MessageBox(0, L"ע����ʧ��", 0, 0);
		return false;
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
		hInstance,						// ����ľ����WinMain �ĵ�һ��������
		NULL							// �������ݡ���һ�㲻�ã����ݸ����ڹ��̵�ֵ
	);

	if (!hwnd)
	{
		MessageBox(0, L"��������ʧ�ܡ�", 0, 0);
		return false;
	}

	// �����ƶ�
	MoveWindow(hwnd, 250, 80, WINDOW_WIDTH, WINDOW_HEIGHT, true);
	// ��ʾ���ڣ� nCmdShow��ʾ������
	ShowWindow(hwnd, nCmdShow);
	// ���´�������
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