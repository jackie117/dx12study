#pragma once

#include <windows.h>

extern float g_posX;
extern float g_posY;
extern HINSTANCE g_hInstance;
extern int g_nCmdShow;
extern HWND g_hwnd;
/// -----------------------------------【宏定义部分】-----------------------------------
#define WINDOWTITLE		L"dx12~"
#define WINDOW_WIDTH	1280
#define WINDOW_HEIGHT	720
/// -----------------------------------------------------------------------------------

#include "initdx.h"

/// -----------------------------------【 函数声明 】----------------------------------
//窗口过程函数的声明,HWND是主窗口句柄
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
bool InitWindow(HINSTANCE hInstance, int nShowCmd);	//初始化窗口
bool Init(HINSTANCE hInstance, int nShowCmd);		//初始化程序
void UpdateScene();	//更新绘制的数据
void DrawScene();	//进行绘制操作
int Run();			//运行
/// ---------------------------------------------------------------------------------
