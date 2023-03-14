#pragma once

#include <windows.h>

extern float g_posX;
extern float g_posY;
extern HINSTANCE g_hInstance;
extern int g_nCmdShow;
extern HWND g_hwnd;
/// -----------------------------------���궨�岿�֡�-----------------------------------
#define WINDOWTITLE		L"dx12~"
#define WINDOW_WIDTH	1280
#define WINDOW_HEIGHT	720
/// -----------------------------------------------------------------------------------

#include "initdx.h"

/// -----------------------------------�� �������� ��----------------------------------
//���ڹ��̺���������,HWND�������ھ��
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
bool InitWindow(HINSTANCE hInstance, int nShowCmd);	//��ʼ������
bool Init(HINSTANCE hInstance, int nShowCmd);		//��ʼ������
void UpdateScene();	//���»��Ƶ�����
void DrawScene();	//���л��Ʋ���
int Run();			//����
/// ---------------------------------------------------------------------------------
