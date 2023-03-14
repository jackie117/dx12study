/// -----------------------------------【头文件包含】-----------------------------------
#include "dx02.h"
/// -----------------------------------------------------------------------------------

// 链接所需的 d3d12 库
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

HWND mhMainWnd = 0;	//某个窗口的句柄，ShowWindow和UpdateWindow函数均要调用此句柄

/// -----------------------------------【全局变量】-----------------------------------
float g_posX = 0;
float g_posY = 0;
HWND g_hwnd = 0;
/// ---------------------------------------------------------------------------------


/// -----------------------------------【 入口函数 WinMain() 】----------------------------------
/// win窗口入口函数
int WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nCmdShow)
{
#if defined(DEBUG) | defined(_DEBUG)
	{
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	}
#endif
	try 
	{
		if (!Init(hInstance, nCmdShow))
		{
			return 0;
		}

		int r = Run();
		return r;
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}
/// ---------------------------------------------------------------------------------

bool Init(HINSTANCE hInstance, int nShowCmd)
{
	if (!InitWindow(hInstance, nShowCmd))
	{
		return false;
	}
	else if (!InitDirect3D())
	{
		return false;
	}
	else
	{
		g_posX = 400.0f;
		g_posY = 1.0f;
		return true;
	}
}

bool InitWindow(HINSTANCE hInstance, int nShowCmd)
{
	// 注册初始化窗口类
	WNDCLASSEX wndClass = { 0 };
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;						// 窗口风格  当工作区宽高改变，则重新绘制窗口
	wndClass.lpfnWndProc = WndProc;									// 消息处理函数回调	默认填 DefWindowProc 默认的消息处理 default Window procedure
	wndClass.hInstance = hInstance;									// 应用程序实例句柄（由WinMain传入）
	wndClass.cbClsExtra = 0;										// 借助这两个字段来为当前应用分配额外的内存空间（这里不分配，所以置0）
	wndClass.cbWndExtra = 0;										// 借助这两个字段来为当前应用分配额外的内存空间（这里不分配，所以置0）
	wndClass.hIcon = LoadIcon(0, IDC_ARROW);						// 窗口图标  //这里使用默认的应用程序图标
	wndClass.hCursor = LoadCursor(0, IDC_ARROW);					// 窗口光标	//使用标准的鼠标指针样式
	wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);	// 指定了白色背景画刷句柄
	wndClass.lpszMenuName = nullptr;								// 菜单栏	//没有菜单栏
	wndClass.lpszClassName = L"ForTheDreamOfGameDevelop";			// 窗口类名
	// 窗口类注册，判断是否失败
	if (!RegisterClassEx(&wndClass))
	{
		// 消息框函数，参数1：消息框所属窗口句柄，可为NULL。
		// 参数2：消息框显示的文本信息。参数3：标题文本。参数4：消息框样式
		MessageBox(0, L"注册类失败", 0, 0);
		return 0;
	}

	//RECT R;	//裁剪矩形
	//R.left = 0;
	//R.top = 0;
	//R.right = WINDOW_WIDTH;
	//R.bottom = WINDOW_HEIGHT;
	//AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);	//根据窗口的客户区大小计算窗口的大小
	//int width = R.right - R.left;
	//int hight = R.bottom - R.top;

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
		return 0;
	}

	// 窗口移动(非必须)
	MoveWindow(hwnd, 250, 80, WINDOW_WIDTH, WINDOW_HEIGHT, true);
	// 显示窗口， g_nCmdShow显示的类型
	ShowWindow(hwnd, nShowCmd);
	// 更新窗口内容
	UpdateWindow(hwnd);
	return true;
}

/// -----------------------------------【消息处理函数】----------------------------------
/// <summary>
///  消息处理函数 。回调函数（操作系统来调用）
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
		//当窗口被销毁时，终止消息循环
		case WM_DESTROY:
		{
			PostQuitMessage(0);	//终止消息循环，并发出WM_QUIT消息
			break;
		}
		default:
			// 将上面没有处理的消息转发给默认的窗口过程
			return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

/// -----------------------------------【进行绘制】-----------------------------------
/// 处理外来信息进行绘制
/// 主要是将我们的各种资源设置到渲染流水线上,并最终发出绘制命令。
void DrawScene()
{
	/// 首先重置命令分配器cmdAllocator和命令列表cmdList，目的是重置命令和列表，复用相关内存。
	ThrowIfFailed(g_commandAllocator->Reset());	//重复使用记录命令的相关内存
	ThrowIfFailed(g_commandList->Reset(g_commandAllocator.Get(), nullptr));	//复用命令列表及其内存
	/// 将后台缓冲资源从呈现状态转换到渲染目标状态（即准备接收图像渲染）。
	UINT& ref_mCurrentBackBuffer = g_mCurrentBackBuffer;
	ID3D12Resource* l_pResourc = g_swapChainBuffer[ref_mCurrentBackBuffer].Get();
	auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(l_pResourc,	//转换资源为后台缓冲区资源
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET); //从呈现到渲染目标转换
	g_commandList->ResourceBarrier(1, &barrier1);
	
	/// 设置视口和裁剪矩形。
	g_commandList->RSSetViewports(1, &g_viewPort);
	g_commandList->RSSetScissorRects(1, &g_scissorRect);
	/// 清除后台缓冲区和深度缓冲区，并赋值。步骤是先获得堆中描述符句柄（即地址），
	/// 再通过ClearRenderTargetView函数和ClearDepthStencilView函数做清除和赋值。
	/// 这里将RT资源背景色赋值为DarkRed（暗红）。
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHeap->GetCPUDescriptorHandleForHeapStart(), ref_mCurrentBackBuffer, g_rtvDescriptorSize);
	g_commandList->ClearRenderTargetView(rtvHandle, DirectX::Colors::DarkRed, 0, nullptr);//清除RT背景色为暗红，并且不设置裁剪矩形
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = g_dsvHeap->GetCPUDescriptorHandleForHeapStart();
	g_commandList->ClearDepthStencilView(dsvHandle,	//DSV描述符句柄
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,	//FLAG
		1.0f,	//默认深度值
		0,	//默认模板值
		0,	//裁剪矩形数量
		nullptr);	//裁剪矩形指针

	/// 指定将要渲染的缓冲区，即指定RTV和DSV。
	g_commandList->OMSetRenderTargets(1,//待绑定的RTV数量
		&rtvHandle,	//指向RTV数组的指针
		true,	//RTV对象在堆内存中是连续存放的
		&dsvHandle);	//指向DSV的指针

	/// 等到渲染完成，将后台缓冲区的状态改成呈现状态，使其之后推到前台缓冲区显示。
	/// 再关闭命令列表，等待传入命令队列。
	l_pResourc = g_swapChainBuffer[ref_mCurrentBackBuffer].Get();
	auto barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(l_pResourc,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT); 
	g_commandList->ResourceBarrier(1, &barrier2);//从渲染目标到呈现
	//完成命令的记录关闭命令列表
	ThrowIfFailed(g_commandList->Close());

	/// 等CPU将命令都准备好后，需要将待执行的命令列表加入GPU的命令队列。使用的是ExecuteCommandLists函数。
	ID3D12CommandList* commandLists[] = { g_commandList.Get() };	//声明并定义命令列表数组
	g_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);	//将命令从命令列表传至命令队列

	/// 交换前后台缓冲区索引（这里的算法是1变0，0变1，为了让后台缓冲区索引永远为0）。
	ThrowIfFailed(g_swapChain1->Present(0, 0));
	ref_mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;

	/// 设置围栏值，刷新命令队列，使CPU和GPU同步。
	FlushCmdQueue();
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
/// 消息循环框架封装在其中
/// 即消息循环选择谁处理
int Run()
{
	// 创建消息结构体
	MSG msg = { 0 };
	// 如果GetMessage函数不等于0，说明没有接受到WM_QUIT
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);		// 键盘按键转换，将虚拟键消息转换为字符消息
			DispatchMessage(&msg);		// 把消息分派给相应的窗口过程
		}
		else
		{
			// 否则就执行动画和游戏逻辑
			UpdateScene();
			DrawScene();
		}
	}
	return (int)msg.wParam;
}
/// ---------------------------------------------------------------------------------