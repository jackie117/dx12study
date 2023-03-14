#include <Windows.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
////////////////////////////////
#include "../common/d3dx12.h"
////////////////////////////////

////////////////////////////////
#include <wrl.h>
////////////////////////////////
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
////////////////////////////////
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <windowsx.h>
#include <comdef.h>
////////////////////////////////

using namespace Microsoft::WRL;

//引用库lib
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")




//AnsiToWString函数（转换成宽字符类型的字符串，wstring）
//在Windows平台上，我们应该都使用wstring和wchar_t，处理方式是在字符串前+L
inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}
//DxException类
class DxException
{
public:
	DxException() = default;
	DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber);

	std::wstring ToString()const;

	HRESULT ErrorCode = S_OK;
	std::wstring FunctionName;
	std::wstring Filename;
	int LineNumber = -1;
};
inline DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
	ErrorCode(hr),
	FunctionName(functionName),
	Filename(filename),
	LineNumber(lineNumber)
{
}

inline std::wstring DxException::ToString()const
{
	// Get the string description of the error code.
	_com_error err(ErrorCode);
	std::wstring msg = err.ErrorMessage();

	return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}

#ifndef ThrowIfFailed
/// 宏定义ThrowIfFailed
/// D3D12大多数创建函数会返回HRESULT，ThrowIfFailed宏可以接收HRESULT值，从而判断是否创建成功。
/// 这个宏展开后，是个可以抛出异常的函数，并能将对应文件名行号显示出来。
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif	// ThrowIfFailed





#define WINDOW_WIDTH	1280
#define WINDOW_HEIGHT	720

// CreateDevice()
ComPtr<IDXGIFactory4> g_dxgiFactory;
ComPtr<ID3D12Device> g_d3dDevice;
// CreateFence()
ComPtr<ID3D12Fence> fence;
// GetDescriptorSize()
UINT g_rtvDescriptorSize;
UINT g_dsvDescriptorSize;
UINT g_cbv_srv_uavDescriptorSize;
// SetMSAA()
D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS g_msaaQualityLevels;
// CreateCommandObjects()
//D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};	// 不需要是全局变量
ComPtr<ID3D12CommandQueue> g_commandQueue;
ComPtr<ID3D12CommandAllocator> g_cmdAllocator;
ComPtr<ID3D12GraphicsCommandList> g_commandList;
// CreateSwapChain()
ComPtr<IDXGISwapChain> g_swapChain;
//DXGI_SWAP_CHAIN_DESC g_swapChainDesc;	// 不需要是全局变量
// CreateDescriptorHeap()
//D3D12_DESCRIPTOR_HEAP_DESC g_rtvDescriptorHeapDesc;	// 不需要是全局变量
ComPtr<ID3D12DescriptorHeap> g_rtvHeap;
//D3D12_DESCRIPTOR_HEAP_DESC g_dsvDescriptorHeapDesc;	// 不需要是全局变量
ComPtr<ID3D12DescriptorHeap> g_dsvHeap;
// CreateRTV()
//CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle;
ComPtr<ID3D12Resource> g_swapChainBuffer[2];
// CreateDSV()
D3D12_RESOURCE_DESC dsvResourceDesc;
ComPtr<ID3D12Resource> depthStencilBuffer;
// FlushCmdQueue()
int mCurrentFence = 0;	//初始CPU上的围栏点为0
// CreateViewPortAndScissorRect()
D3D12_VIEWPORT g_viewPort;
D3D12_RECT g_scissorRect;
// Draw()
UINT g_mCurrentBackBuffer = 0;
//声明主窗口句柄
HWND g_hwnd = 0;	//某个窗口的句柄，ShowWindow和UpdateWindow函数均要调用此句柄


//bool InitWindow(HINSTANCE hInstance, int nShowCmd);
void Draw();
int Run();
bool Init(HINSTANCE hInstance, int nShowCmd);
//窗口过程函数的声明,HWND是主窗口句柄
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lparam);

/// 书写WinMain主函数。主函数相当于C++中的Main入口函数。在主函数中首先是一个调试程序的固定写法。
int WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nCmdShow)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	try
	{
		if (!Init(hInstance, nCmdShow))
		{
			return 0;
		}

		return Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}

}

/// -----------------------------------【消息处理函数】----------------------------------
/// <summary>
/// 消息处理函数 。回调函数（操作系统来调用）
/// 将接收到的消息分派给窗口过程
/// </summary>
/// <param name="hwnd"></param>
/// <param name="message">消息类型</param>
/// <param name="wParam">消息内容</param>
/// <param name="lParam">消息内容</param>
/// <returns></returns>
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//消息处理
	switch (message)
	{
		//当窗口被销毁时，终止消息循环
	case WM_DESTROY:
		PostQuitMessage(0);	//终止消息循环，并发出WM_QUIT消息
		break;
	default:
		//将上面没有处理的消息转发给默认的窗口过程
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}


//0.开启D3D12调试层。（在初始化D3D的开头）
//创建设备。			CreateDevice()
//创建围栏。			CreateFence()
//获取描述符大小。	GetDescriptorSize()
//设置MSAA抗锯齿属性。
//创建命令队列、命令列表、命令分配器。
//创建交换链。
//创建描述符堆。
//创建描述符。
//资源转换。
//设置视口和裁剪矩形。
//设置围栏刷新命令队列。
//将命令从列表传至队列。


/// 1.创建设备
/// 我们将各个模块封装成函数。首先是创建设备。
/// 先来了解下DXGI API，设计DXGI的基本理念是使用多种图形API中所共有的底层任务能借助一组通用API来进行处理。
/// 而我们的设备即是通过DXGI API的IDXGIFactory接口来创建的。所以我们先要CreateDXGIFactory，
/// 然后再CreateDevice。
void CreateDevice()
{
	//ComPtr<IDXGIFactory4> dxgiFactory;
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&g_dxgiFactory)));
	//ComPtr<ID3D12Device> g_d3dDevice;
	// 
	// 创建D3D12.1的设备
	ThrowIfFailed(D3D12CreateDevice(
		nullptr,                    //此参数如果设置为nullptr，则使用主适配器
		D3D_FEATURE_LEVEL_12_1,		//应用程序需要硬件所支持的最低功能级别
		IID_PPV_ARGS(&g_d3dDevice)));	//返回所建设备
}


/// 2.通过设备创建围栏fence，以便之后同步CPU和GPU。
/// CPU可以通过围栏来判断GPU是否完成了某些操作。
void CreateFence()
{
	//ComPtr<ID3D12Fence> fence;
	ThrowIfFailed(g_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
}

/// 3.获取描述符大小
/// 可以让我们知道描述符堆中每个元素的大小（描述符在不同的GPU平台上的大小各异），
/// 方便我们之后在地址中做偏移来找到堆中的描述符元素。
/// 这里我们获取三个描述符大小，分别是
///  RTV（渲染目标缓冲区描述符）、
///  DSV（深度模板缓冲区描述符）、
///  CBV_SRV_UAV（常量缓冲区描述符、着色器资源缓冲描述符和随机访问缓冲描述符）。
void GetDescriptorSize()
{
	//UINT 
	g_rtvDescriptorSize = g_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//UINT 
	g_dsvDescriptorSize = g_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	//UINT 
	g_cbv_srv_uavDescriptorSize = g_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

/// 4.设置MSAA抗锯齿属性。
/// 先填充多重采样属性结构体，然后通过CheckFeatureSupport函数设置NumQualityLevels。
/// 注意：此处不使用MSAA，采样数量设置为0。
void SetMSAA()
{
	//D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS g_msaaQualityLevels;
	g_msaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	//UNORM是归一化处理的无符号整数
	g_msaaQualityLevels.SampleCount = 1;
	g_msaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	g_msaaQualityLevels.NumQualityLevels = 0;
	//当前图形驱动对MSAA多重采样的支持（注意：第二个参数即是输入又是输出）
	ThrowIfFailed(g_d3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &g_msaaQualityLevels, sizeof(g_msaaQualityLevels)));
	//NumQualityLevels在Check函数里会进行设置
	//如果支持MSAA，则Check函数返回的NumQualityLevels > 0
	//expression为假（即为0），则终止程序运行，并打印一条出错信息
	assert(g_msaaQualityLevels.NumQualityLevels > 0);
}

/// 5.创建命令队列、命令分配器和命令列表。
/// 他们三者的关系是：
///  首先CPU创建命令列表，
///  然后将关联在命令分配器上的命令传入命令列表，
///  最后将命令列表传入命令队列给GPU处理。这一步只是做了三者的创建工作。
/// 注意，我们在初始化D3D12_COMMAND_QUEUE_DESC时，
/// 只初始化了两项，其他两项我们在大括号中默认初始化了。
void CreateCommandObject()
{
	//创建命令队列
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;	//指定命令队列类型
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;	//指定命令队列的属性
	//ComPtr<ID3D12CommandQueue> g_commandQueue;
	ThrowIfFailed(g_d3dDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&g_commandQueue)));

	//创建命令列表分配器
	//ComPtr<ID3D12CommandAllocator> g_cmdAllocator;
	ThrowIfFailed(g_d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_cmdAllocator)));//&cmdAllocator等价于cmdAllocator.GetAddressOf

	//创建命令列表
	//ComPtr<ID3D12GraphicsCommandList> g_commandList;	
	//注意此处的接口名是ID3D12GraphicsCommandList，而不是ID3D12CommandList
	ThrowIfFailed(g_d3dDevice->CreateCommandList(
		0,								//掩码值为0，单GPU
		D3D12_COMMAND_LIST_TYPE_DIRECT, //命令列表类型
		g_cmdAllocator.Get(),			//命令分配器接口指针
		nullptr,						//初始管线(流水线)状态对象PSO，这里不绘制，所以空指针
		IID_PPV_ARGS(&g_commandList)));	//返回创建的命令列表

	// 关闭命令列表，因为我们只需要一次，所以在创建完后就关闭
	g_commandList->Close();				//重置命令列表前必须将其关闭
}

/// 6.创建交换链
/// 交换链中存着渲染目标资源，即后台缓冲区资源。通过DXGI API下的IDXGIFactory接口来创建交换链。
/// 这里还是禁用MSAA多重采样。因为其设置比较麻烦，这里直接设置MSAA会出错，所以count还是为1，质量为0。
/// 还要注意一点，CreateSwapChain函数的第一个参数其实是命令队列接口指针，不是设备接口指针，参数描述有误导。
void CreateSwapChain()
{
	//ComPtr<IDXGISwapChain> swapChain;
	// 释放之前创建的交换链，然后进行重建
	g_swapChain.Reset();

	DXGI_SWAP_CHAIN_DESC swapChainDesc;	// 交换链描述结构体
	swapChainDesc.BufferDesc.Width = 1280;	// 缓冲区分辨率的宽度
	swapChainDesc.BufferDesc.Height = 720;	// 缓冲区分辨率的高度
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	//缓冲区的显示格式 设置格式为 RGBA 格式
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;	// 刷新率的分子
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;	// 刷新率的分母
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;	// 扫描线顺序  逐行扫描VS隔行扫描(未指定的)
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;	// 缩放模式 图像相对屏幕的拉伸（未指定的）
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// 缓冲区用途 将数据渲染至后台缓冲区（即作为渲染目标）
	swapChainDesc.OutputWindow = g_hwnd;	// 渲染(输出)窗口句柄
	swapChainDesc.SampleDesc.Count = 1;		// 多重采样数量
	swapChainDesc.SampleDesc.Quality = 0;	// 多重采样质量
	swapChainDesc.Windowed = true;			// 是否窗口化
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	// 交换效果 固定写法
	swapChainDesc.BufferCount = 2;			// 后台缓冲区数量（双缓冲）
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	// 交换链标志 自适应窗口模式（自动选择最适于当前窗口尺寸的显示模式）

	// 利用DXGI接口下的工厂类 创建交换链
	ThrowIfFailed(g_dxgiFactory->CreateSwapChain(
		g_commandQueue.Get(), 		// 命令队列接口指针 不能为NULL
		&swapChainDesc, 			// 交换链描述符 不能为NULL
		g_swapChain.GetAddressOf()));	// 用于接受交换链

	//DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
	//swapChainDesc.Width = WINDOW_WIDTH;
	//swapChainDesc.Height = WINDOW_HEIGHT;
	//swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	//swapChainDesc.Stereo = FALSE;
	//swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	//swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	//swapChainDesc.SampleDesc.Count = 1;
	//swapChainDesc.SampleDesc.Quality = 0;
	//swapChainDesc.BufferCount = 2;
	//swapChainDesc.Scaling = DXGI_SCALING_STRETCH; //DXGI_SCALING_ASPECT_RATIO_STRETCH
	//swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	//swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	//// 创建交换链
	//ThrowIfFailed(g_dxgiFactory->CreateSwapChainForHwnd(
	//	g_commandQueue.Get(),	// 命令队列接口指针 不能为NULL
	//	g_hwnd,					// 窗口句柄 不能为NULL
	//	&swapChainDesc,			// 交换链描述符 不能为NULL
	//	nullptr,				// 全屏状态下的窗口状态 为NULL时使用默认值
	//	nullptr,				// 全屏状态下的输出 为NULL时使用默认值
	//	&g_swapChain1			// 用于接受交换链   g_swapChain1.GetAddressOf()
	//));
}


/// 创建描述符堆（descriptorHeap），
/// 描述符堆是存放描述符的一段连续内存空间。因为是双后台缓冲，所以要创建存放2个RTV的RTV堆，
/// 而深度模板缓存只有一个，所以创建1个DSV的DSV堆。
/// 具体过程: 先填充描述符堆属性结构体，然后通过设备创建描述符堆。
void CreateDescriptorHeap()
{
	//首先创建RTV堆
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc;
	rtvDescriptorHeapDesc.NumDescriptors = 2;		// 描述符数量
	rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// 描述符堆标志
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	// 描述符堆类型
	rtvDescriptorHeapDesc.NodeMask = 0;	// 节点掩码
	//ComPtr<ID3D12DescriptorHeap> g_rtvHeap;
	ThrowIfFailed(g_d3dDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&g_rtvHeap)));

	//然后创建DSV堆
	D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc;
	dsvDescriptorHeapDesc.NumDescriptors = 1;		// 描述符数量
	dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// 描述符堆标志
	dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;	// 描述符堆类型
	dsvDescriptorHeapDesc.NodeMask = 0;	// 节点掩码
	//ComPtr<ID3D12DescriptorHeap> g_dsvHeap;
	ThrowIfFailed(g_d3dDevice->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(&g_dsvHeap)));
}

/// 创建堆中的描述符
/// 先创建RTV。具体过程是，
///  先从RTV堆中拿到首个RTV句柄，
///  然后获得存于交换链中的RT资源，
///  最后创建RTV将RT资源和RTV句柄联系起来，
///  并在最后根据RTV大小做了在堆中的地址偏移。
///注意这里用到了CD3DX12_CPU_DESCRIPTOR_HANDL，
///这个变体在d3dx12.h头文件中定义，DX库并没有集成，
///需要我们自行下载。这里的CD3DX12_CPU_DESCRIPTOR_HANDLE是个变体类，
///它的构造函数初始化了D3D12_CPU_DESCRIPTOR_HANDLE结构体中的元素，
///所以直接调用其构造函数即可。
///之后我们会经常用到CD3DX12开头的变体类来简化初始化的代码书写，
///用法和作用基本一致。
void CreateRTV()
{
	// 获取RTV描述符堆的起始CPU描述符句柄
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(g_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	//ComPtr<ID3D12Resource> g_swapChainBuffer[2];
	// 获取后台缓冲区资源
	for (int i = 0; i < 2; i++)
	{
		//获得存于交换链中的后台缓冲区资源
		g_swapChain->GetBuffer(i, IID_PPV_ARGS(g_swapChainBuffer[i].GetAddressOf()));
		//创建RTV描述符
		g_d3dDevice->CreateRenderTargetView(g_swapChainBuffer[i].Get(),
			nullptr,	//在交换链创建中已经定义了该资源的数据格式，所以这里指定为空指针
			rtvHeapHandle);	//描述符句柄结构体（这里是变体，继承自CD3DX12_CPU_DESCRIPTOR_HANDLE）
		//偏移到描述符堆中的下一个缓冲区
		rtvHeapHandle.Offset(1, g_rtvDescriptorSize);
	}
}


/// 创建DSV
/// 具体过程:
///  先在CPU中创建好DS资源，
///  然后通过CreateCommittedResource函数将DS资源提交至GPU显存中，
///  最后创建DSV将显存中的DS资源和DSV句柄联系起来。
void CreateDSV()
{
	//在CPU中创建好深度模板数据资源
	//D3D12_RESOURCE_DESC dsvResourceDesc;
	dsvResourceDesc.Alignment = 0;	//指定对齐
	dsvResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	//指定资源维度（类型）为TEXTURE2D
	dsvResourceDesc.DepthOrArraySize = 1;	//  资源深度或数组大小  纹理深度为1
	dsvResourceDesc.Width = WINDOW_WIDTH;	// 资源宽
	dsvResourceDesc.Height = WINDOW_HEIGHT;	// 资源高
	dsvResourceDesc.MipLevels = 1;			// 资源的最大层级 MIPMAP层级数量
	dsvResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;	//指定纹理布局（这里不指定）
	dsvResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;	//深度模板资源的Flag
	dsvResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;	//24位深度，8位模板,还有个无类型的格式DXGI_FORMAT_R24G8_TYPELESS也可以使用
	dsvResourceDesc.SampleDesc.Count = 1;	//多重采样数量  （这个值和swapChainDesc 对应）
	dsvResourceDesc.SampleDesc.Quality = g_msaaQualityLevels.NumQualityLevels - 1;	//多重采样质量等级

	// 创建深度模板数据资源
	CD3DX12_CLEAR_VALUE optClear = {};	// 清除资源的优化值，提高清除操作的执行速度（CreateCommittedResource函数中传入）
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//24位深度，8位模板,还有个无类型的格式DXGI_FORMAT_R24G8_TYPELESS也可以使用
	optClear.DepthStencil.Depth = 1;	//初始深度值为 1.0f
	optClear.DepthStencil.Stencil = 0;	//初始模板值为0
	//创建一个资源和一个堆，并将资源提交至堆中（将深度模板数据提交至GPU显存中）
	//ComPtr<ID3D12Resource> depthStencilBuffer;
	auto l_heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(g_d3dDevice->CreateCommittedResource(
		&l_heapProperties,			// 堆类型为默认堆（不能写入）D3D12_HEAP_TYPE_UPLOAD
		D3D12_HEAP_FLAG_NONE,		//Flag
		&dsvResourceDesc,			//上面定义的DSV资源指针
		D3D12_RESOURCE_STATE_COMMON,//资源的状态为初始状态
		&optClear,					//上面定义的优化值指针
		IID_PPV_ARGS(&depthStencilBuffer)));	//返回深度模板资源

	//创建DSV(必须填充DSV属性结构体，和创建RTV不同，RTV是通过句柄)
		//D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		//dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		//dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		//dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		//dsvDesc.Texture2D.MipSlice = 0;
	/// 创建DSV描述符
	g_d3dDevice->CreateDepthStencilView(depthStencilBuffer.Get(),
		nullptr,	//D3D12_DEPTH_STENCIL_VIEW_DESC类型指针，可填&dsvDesc（见上注释代码），
		//由于在创建深度模板资源时已经定义深度模板数据属性，所以这里可以指定为空指针
		g_dsvHeap->GetCPUDescriptorHandleForHeapStart());	//DSV句柄
}

/// 标记DS资源的状态。
/// 因为资源在不同的时间段有着不同的作用，
/// 比如，有时候它是只读，有时候又是可写入的。
/// 我们用ResourceBarrier下的Transition函数来转换资源状态。
void ResourceBarrierBuild()
{
	// 标记DS资源的状态
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(depthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON,		//转换前状态（创建时的状态，即CreateCommittedResource函数中定义的状态）
		D3D12_RESOURCE_STATE_DEPTH_WRITE);	//转换后状态为可写入的深度图，还有一个D3D12_RESOURCE_STATE_DEPTH_READ是只可读的深度图
	g_commandList->ResourceBarrier(1,		//Barrier屏障个数
		&barrier);

	//等所有命令都进入cmdList后，还需要用ExecuteCommandLists函数，
	//将命令从命令列表传入命令队列，也就是从CPU传入GPU的过程。
	//注意：在传入命令队列前必须关闭命令列表。
	ThrowIfFailed(g_commandList->Close());	//命令添加完后将其关闭
	ID3D12CommandList* cmdLists[] = { g_commandList.Get() };	//声明并定义命令列表数组
	g_commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);	//将命令从命令列表传至命令队列

}

//为了使CPU和GPU同步，我们之前已经创建了围栏接口，现在需要实现围栏代码。
//实现思想是，围栏初始值为0（可理解为GPU端的围栏值），
//现我们定义一个当前围栏值也为0（可理解为CPU端的围栏值），
//当CPU将命令传递至GPU后，当前围栏值++（CPU围栏++），
//而当GPU处理完CPU传来的命令后，围栏值++（GPU围栏++），
//然后判定围栏值（CPU围栏值）和当前围栏值（GPU围栏值）的大小，来确定GPU是否命中围栏点，
//如果没有命中，则等待命中后触发事件。

/// 11.设置围栏刷新命令队列。
/// 12.将命令从列表传至队列。
void FlushCmdQueue()
{
	mCurrentFence++;	//CPU传完命令并关闭后，将当前围栏值+1

	// 将命令队列中的命令传入Fence中
	//当GPU处理完CPU传入的命令后，将fence接口中的围栏值+1，即fence->GetCompletedValue()+1
	ThrowIfFailed(g_commandQueue->Signal(fence.Get(), mCurrentFence));

	// 如果Fence的完成值小于当前围栏（帧）的Fence值，说明GPU还没有执行完当前帧的命令
	if (fence->GetCompletedValue() < mCurrentFence)	//如果小于，说明GPU没有处理完所有命令
	{
		// 创建一个事件句柄
		HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenceSetDone");

		// 将Fence的完成值设置为当前帧的Fence值
		// 当围栏达到mCurrentFence值（即执行到Signal（）指令修改了围栏值）时触发的eventHandle事件
		ThrowIfFailed(fence->SetEventOnCompletion(mCurrentFence, eventHandle));

		//等待GPU命中围栏，激发事件（阻塞当前线程直到事件触发，注意此Enent需先设置再等待，
		//如果没有Set就Wait，就死锁了，Set永远不会调用，所以也就没线程可以唤醒这个线程）
		WaitForSingleObject(eventHandle, INFINITE);

		// 关闭事件句柄
		CloseHandle(eventHandle);
	}
}

/// 10.设置视口和裁剪矩形。
void CreateViewPortAndScissorRect()
{
	//D3D12_VIEWPORT viewPort;
	//D3D12_RECT scissorRect;
	//视口设置
	g_viewPort.TopLeftX = 0;
	g_viewPort.TopLeftY = 0;
	g_viewPort.Width = static_cast<float>(WINDOW_WIDTH);
	g_viewPort.Height = static_cast<float>(WINDOW_HEIGHT);
	g_viewPort.MaxDepth = 1.0f;
	g_viewPort.MinDepth = 0.0f;

	//裁剪矩形设置（矩形外的像素都将被剔除）
	//前两个为左上点坐标，后两个为右下点坐标
	g_scissorRect.left = 0;
	g_scissorRect.top = 0;
	g_scissorRect.right = WINDOW_WIDTH;
	g_scissorRect.bottom = WINDOW_HEIGHT;
}

/// 我们对之前的代码略微做点封装，但是为了利于逻辑的理解，不会一下子封装的很深。
/// 首先，我们建一个InitWindow函数，
/// 然后将之前写的创建窗口代码拷贝进去。
/// 此函数返回一个布尔值，如果窗口创建成功则返回true。
bool InitWindow(HINSTANCE hInstance, int nShowCmd)
{
	// 窗口初始化描述结构体(WNDCLASS)
	WNDCLASSEX wndClass = {};
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;	// 窗口风格  当工作区宽高改变，则重新绘制窗口
	wndClass.lpfnWndProc = MainWndProc;			// 消息处理函数回调	默认填 DefWindowProc 默认的消息处理 default Window procedure
	wndClass.cbClsExtra = 0;	// 借助这两个字段来为当前应用分配额外的内存空间（这里不分配，所以置0）
	wndClass.cbWndExtra = 0;	// 借助这两个字段来为当前应用分配额外的内存空间（这里不分配，所以置0）
	wndClass.hInstance = hInstance;	// 应用程序实例句柄（由WinMain传入）
	wndClass.hIcon = LoadIcon(0, IDC_ARROW);	// 窗口图标 // 使用默认的应用程序图标
	wndClass.hCursor = LoadCursor(0, IDC_ARROW);// 窗口光标// 使用标准的鼠标指针样式
	wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);	//指定了白色背景画刷句柄 防止无聊的背景重绘
	wndClass.lpszMenuName = 0;				// 没有菜单栏
	wndClass.lpszClassName = L"MainWnd";	// 窗口类名
	//窗口类注册失败
	if (!RegisterClassEx(&wndClass))
	{
		//消息框函数，参数1：消息框所属窗口句柄，可为NULL。参数2：消息框显示的文本信息。参数3：标题文本。参数4：消息框样式
		MessageBox(0, L"RegisterClass Failed", 0, 0);
		return 0;
	}

	RECT R;	//裁剪矩形
	R.left = 0;
	R.top = 0;
	R.right = WINDOW_WIDTH;
	R.bottom = WINDOW_HEIGHT;
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);	//根据窗口的客户区大小计算窗口的大小
	int width = R.right - R.left;
	int hight = R.bottom - R.top;

	//创建窗口,返回布尔值
	//CreateWindowW(lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam)
	g_hwnd = CreateWindowExW(
		WS_EX_OVERLAPPEDWINDOW,			// 扩展窗口样式
		L"MainWnd", L"DX12Initialize", 	// 窗口类名 & 窗口标题
		WS_OVERLAPPEDWINDOW, 			// 要创建的窗口类型
		CW_USEDEFAULT, CW_USEDEFAULT,	// 初始位置(x, y)（相对与父窗口）
		width, hight,					// 初始大小（宽度，长度）
		NULL,							// 此窗口的父级（父窗口句柄）
		NULL,							// 菜单栏句柄
		hInstance,						// 程序的句柄（WinMain 的第一个参数）
		NULL							// 附加数据。（一般不用）传递给窗口过程的值
	);
	//p窗口创建失败
	if (!g_hwnd)
	{
		MessageBox(0, L"CreatWindow Failed", 0, 0);
		return 0;
	}

	// 窗口移动(非必须)
	//MoveWindow(g_hwnd, 250, 80, WINDOW_WIDTH, WINDOW_HEIGHT, true);
	// 显示窗口， g_nCmdShow显示的类型
	ShowWindow(g_hwnd, nShowCmd);
	// 更新窗口内容
	UpdateWindow(g_hwnd);

	return true;
}

//Draw函数主要是将我们的各种资源设置到渲染流水线上, 
//并最终发出绘制命令。首先重置命令分配器cmdAllocator和命令列表cmdList，
//目的是重置命令和列表，复用相关内存。
void Draw()
{
	/// 首先重置命令分配器cmdAllocator和命令列表cmdList，目的是重置命令和列表，复用相关内存。
	ThrowIfFailed(g_cmdAllocator->Reset());		//重复使用记录命令的相关内存
	ThrowIfFailed(g_commandList->Reset(g_cmdAllocator.Get(), nullptr));	//复用命令列表及其内存
	// 将后台缓冲资源从呈现状态转换到渲染目标状态（即准备接收图像渲染）
	UINT& ref_mCurrentBackBuffer = g_mCurrentBackBuffer;
	auto l_barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(
		g_swapChainBuffer[ref_mCurrentBackBuffer].Get(),  //转换资源为后台缓冲区资源
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);  //从呈现到渲染目标转换
	g_commandList->ResourceBarrier(1, &l_barrier1);

	//接下来设置视口和裁剪矩形。
	g_commandList->RSSetViewports(1, &g_viewPort);
	g_commandList->RSSetScissorRects(1, &g_scissorRect);


	// 清除后台缓冲区和深度缓冲区，并赋值。
	//步骤是先获得堆中描述符句柄（即地址），
	//再通过ClearRenderTargetView函数和ClearDepthStencilView函数做清除和赋值。
	//这里我们将RT资源背景色赋值为Black（黑色）DarkRed（暗红）。
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHeap->GetCPUDescriptorHandleForHeapStart(), ref_mCurrentBackBuffer, g_rtvDescriptorSize);
	g_commandList->ClearRenderTargetView(
		rtvHandle,
		DirectX::Colors::Black,
		0,
		nullptr);	//清除RT背景色为黑色，并且不设置裁剪矩形
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = g_dsvHeap->GetCPUDescriptorHandleForHeapStart();
	g_commandList->ClearDepthStencilView(
		dsvHandle,	//DSV描述符句柄
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,	//FLAG
		1.0f,	//默认深度值
		0,	//默认模板值
		0,	//裁剪矩形数量
		nullptr);	//裁剪矩形指针

	//然后我们指定将要渲染的缓冲区，即指定RTV和DSV。
	g_commandList->OMSetRenderTargets(1,//待绑定的RTV数量
		&rtvHandle,	//指向RTV数组的指针
		true,	//RTV对象在堆内存中是连续存放的
		&dsvHandle);	//指向DSV的指针

	//等到渲染完成，我们要将后台缓冲区的状态改成呈现状态，使其之后推到前台缓冲区显示。
	//完了，关闭命令列表，等待传入命令队列。
	auto l_barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
		g_swapChainBuffer[ref_mCurrentBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	g_commandList->ResourceBarrier(1, &l_barrier2);//从渲染目标到呈现
	//完成命令的记录关闭命令列表
	ThrowIfFailed(g_commandList->Close());

	//等CPU将命令都准备好后，需要将待执行的命令列表加入GPU的命令队列。
	//使用的是ExecuteCommandLists函数。
	ID3D12CommandList* commandLists[] = { g_commandList.Get() };//声明并定义命令列表数组
	g_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);//将命令从命令列表传至命令队列

	//然后交换前后台缓冲区索引（这里的算法是1变0，0变1，为了让后台缓冲区索引永远为0）。
	ThrowIfFailed(g_swapChain->Present(0, 0));
	ref_mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;

	//最后设置围栏值，刷新命令队列，使CPU和GPU同步，这段代码在第一篇中有详细解释，这里直接封装。
	FlushCmdQueue();


}

//然后我们新建一个Run函数，将之前的消息循环代码复制进去，
//并将其结构略作调整。
//因为考虑到我们是游戏程序，
//所以调整了分支结构，
//如果没有消息处理，就执行游戏画面和逻辑的计算，
//这里的Draw函数之后会定义
//（Draw函数每运行一次，其实就是一帧，所以后期会在它前面加上帧时间的计算）。
int Run()
{
	//消息循环
	//定义消息结构体
	MSG msg = { 0 };
	//如果GetMessage函数不等于0，说明没有接受到WM_QUIT
	while (msg.message != WM_QUIT)
	{
		//如果有窗口消息就进行处理
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) //PeekMessage函数会自动填充msg结构体元素
		{
			TranslateMessage(&msg);	//键盘按键转换，将虚拟键消息转换为字符消息
			DispatchMessage(&msg);	//把消息分派给相应的窗口过程
		}
		//否则就执行动画和游戏逻辑
		else
		{
			Draw();
		}
	}
	return (int)msg.wParam;
}

//接下来我们建一个InitDirect3D函数，
//将之前我们列出的初始化步骤，
//除了最后两条（这两条在Draw函数中执行），
//都拷贝至函数中（这里对之前的步骤都做了函数封装）。
bool InitDirect3D()
{
	/*开启D3D12调试层*/
#if defined(DEBUG) || defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif

	CreateDevice();
	CreateFence();
	GetDescriptorSize();
	SetMSAA();
	CreateCommandObject();
	CreateSwapChain();
	CreateDescriptorHeap();
	CreateRTV();
	CreateDSV();
	CreateViewPortAndScissorRect();

	return true;
}

//接下来我们建一个总的Init函数，
//将InitWindow和InitDirect3D放一起做一个判断，
//如果两个初始化都正常执行，
//则判断总的初始化正常执行，返回true。
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
		return true;
	}
}
