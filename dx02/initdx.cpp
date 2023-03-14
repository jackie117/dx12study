#include "dx02.h"

ComPtr<IDXGIFactory4> g_dxgiFactory;
ComPtr<ID3D12Device> g_d3dDevice;
ComPtr<ID3D12Fence> g_fence;
ComPtr<ID3D12CommandQueue> g_commandQueue;
ComPtr<ID3D12CommandAllocator> g_commandAllocator;
ComPtr<ID3D12GraphicsCommandList> g_commandList;
ComPtr<IDXGISwapChain1> g_swapChain1;
ComPtr<ID3D12DescriptorHeap> g_rtvHeap;
ComPtr<ID3D12DescriptorHeap> g_dsvHeap;
ComPtr<ID3D12Resource> g_swapChainBuffer[2];
ComPtr<ID3D12Resource> g_depthStencilBuffer;

D3D12_VIEWPORT g_viewPort;
D3D12_RECT g_scissorRect;
UINT g_rtvDescriptorSize = 0;
UINT g_dsvDescriptorSize = 0;
UINT g_cbv_srv_uavDescriptorSize = 0;
UINT g_mCurrentBackBuffer = 0;

unsigned g_currentFence;
D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS g_msaaQualityLevels;

//AnsiToWString函数（转换成宽字符类型的字符串，wstring）
//在Windows平台上，我们应该都使用wstring和wchar_t，处理方式是在字符串前+L
inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}

DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
	ErrorCode(hr),
	FunctionName(functionName),
	Filename(filename),
	LineNumber(lineNumber)
{
}


std::wstring DxException::ToString()const
{
	// Get the string description of the error code.
	_com_error err(ErrorCode);
	std::wstring msg = err.ErrorMessage();

	return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}


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

/// 创建设备
/// 设备即是通过DXGI API的IDXGIFactory接口来创建
void CreateDevice()
{
	//ComPtr<IDXGIFactory4> dxgiFactory;
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&g_dxgiFactory)));
	// ComPtr<ID3D12Device> d3dDevice;
	ThrowIfFailed(D3D12CreateDevice(
		nullptr,					//此参数如果设置为nullptr，则使用主适配器
		D3D_FEATURE_LEVEL_12_0,		//应用程序需要硬件所支持的最低功能级别
		IID_PPV_ARGS(&g_d3dDevice))	//返回所建设备
	);
}

/// 创建围栏，同步CPU和GPU。
/// 围栏是用来同步CPU和GPU的，CPU可以通过围栏来判断GPU是否完成了某些操作。
void CreateFence()
{
	// ComPtr<ID3D12Fence> fence;
	ThrowIfFailed(g_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)));
}

/// 获取描述符大小。
/// 可以让我们知道描述符堆中每个元素的大小（描述符在不同的GPU平台上的大小各异），
/// 方便我们之后在地址中做偏移来找到堆中的描述符元素。/// 这里获取三个描述符大小，
/// 分别是RTV（渲染目标缓冲区描述符）、DSV（深度模板缓冲区描述符）、
/// CBV_SRV_UAV（常量缓冲区描述符、着色器资源缓冲描述符和随机访问缓冲描述符）。
void GetDescriptorSize()
{
	g_rtvDescriptorSize = g_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	g_dsvDescriptorSize = g_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	g_cbv_srv_uavDescriptorSize = g_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

/// 设置MSAA抗锯齿属性。
void SetMSAA()
{
	//D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQualityLevels;
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

/// 创建命令队列、命令列表、命令分配器。
void CreateCommandObject()
{
	//创建命令队列
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;	//指定命令队列类型
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;	//指定命令队列的属性
	// ComPtr<ID3D12CommandQueue> cmdQueue;
	ThrowIfFailed(g_d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_commandQueue)));

	//创建命令分配器
	// ComPtr<ID3D12CommandAllocator> cmdAllocator;
	ThrowIfFailed(g_d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_commandAllocator)));

	//创建命令列表
	// ComPtr<ID3D12GraphicsCommandList> cmdList;
	// 注意此处的接口名是ID3D12GraphicsCommandList，而不是ID3D12CommandList
	ThrowIfFailed(g_d3dDevice->CreateCommandList(
		0,								// 掩码值为0，单GPU
		D3D12_COMMAND_LIST_TYPE_DIRECT, // 命令列表类型
		g_commandAllocator.Get(), 		// 命令分配器接口指针
		nullptr, 						// 初始管线(流水线)状态对象PSO，这里不绘制，所以空指针
		IID_PPV_ARGS(&g_commandList))	// 返回的命令列表接口指针
	);
	// 关闭命令列表，因为我们只需要一次，所以在创建完后就关闭
	g_commandList->Close();		// 重置命令列表前必须将其关闭
}

/// 创建交换链。
/// 交换链中存着渲染目标资源，即后台缓冲区资源。通过DXGI API下的IDXGIFactory接口来创建交换链。
/// 还是禁用MSAA多重采样。因为其设置比较麻烦，这里直接设置MSAA会出错，所以count还是为1，质量为0。
/// 还要注意，CreateSwapChain函数的第一个参数其实是命令队列接口指针，不是设备接口指针，参数描述有误导。
void CreateSwapChain()
{
	// ComPtr<IDXGISwapChain> swapChain;
	// 释放之前创建的交换链，然后进行重建
	g_swapChain1.Reset();

	//DXGI_SWAP_CHAIN_DESC swapChainDesc = {};	// 交换链描述符结构体
	//swapChainDesc.BufferDesc.Width = WINDOW_WIDTH;			// 缓冲区分辨率的宽度
	//swapChainDesc.BufferDesc.Height = WINDOW_HEIGHT;		// 缓冲区分辨率的高度
	//swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;	// 刷新率的分子
	//swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;	// 刷新率的分母
	//swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	// 缓冲区的显示格式 设置格式为 RGBA 格式
	//swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;	// 扫描线顺序  逐行扫描VS隔行扫描(未指定的)
	//swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;	// 缩放模式 图像相对屏幕的拉伸（未指定的）
	//swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// 缓冲区用途 将数据渲染至后台缓冲区（即作为渲染目标）
	//swapChainDesc.SampleDesc.Count = 1;		// 多重采样数量
	//swapChainDesc.SampleDesc.Quality = 0;	// 多重采样质量等级
	//swapChainDesc.BufferCount = 2;			// 缓冲区数量
	//swapChainDesc.OutputWindow = g_hwnd;	// 输出(渲染)窗口句柄
	//swapChainDesc.Windowed = true;			// 是否窗口化
	//swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	// 交换效果
	//swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	// 交换链标志 自适应窗口模式（自动选择最适于当前窗口尺寸的显示模式）

	//// 创建交换链
	//ThrowIfFailed(g_dxgiFactory->CreateSwapChain(
	//	g_commandQueue.Get(),	// 命令队列接口指针 不能为NULL
	//	&swapChainDesc,			// 交换链描述符 不能为NULL
	//	g_swapChain.GetAddressOf()	// 用于接受交换链
	//));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
	swapChainDesc.Width = WINDOW_WIDTH;
	swapChainDesc.Height = WINDOW_HEIGHT;
	swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH; //DXGI_SCALING_ASPECT_RATIO_STRETCH
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	// 创建交换链
	ThrowIfFailed(g_dxgiFactory->CreateSwapChainForHwnd(
		g_commandQueue.Get(),	// 命令队列接口指针 不能为NULL
		g_hwnd,					// 窗口句柄 不能为NULL
		&swapChainDesc,			// 交换链描述符 不能为NULL
		nullptr,				// 全屏状态下的窗口状态 为NULL时使用默认值
		nullptr,				// 全屏状态下的输出 为NULL时使用默认值
		g_swapChain1.GetAddressOf()	// 用于接受交换链
	));
}

/// 创建描述符堆。
/// 存放描述符的一段连续内存空间。因为是双后台缓冲，所以我们要创建存放2个RTV的RTV堆，
/// 而深度模板缓存只有一个，所以创建1个DSV的DSV堆。
/// 先填充描述符堆属性结构体，然后通过设备创建描述符堆。
void CreateDescriptorHeap()
{
	// 创建RTV描述符堆
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = 2;		// 描述符数量
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	// 描述符类型
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// 描述符堆标志
	rtvHeapDesc.NodeMask = 0;	// 节点掩码
	//ComPtr<ID3D12DescriptorHeap> rtvHeap;
	ThrowIfFailed(g_d3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&g_rtvHeap)));

	// 创建DSV描述符堆
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;		// 描述符数量
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;	// 描述符类型
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// 描述符堆标志
	dsvHeapDesc.NodeMask = 0;	// 节点掩码
	//ComPtr<ID3D12DescriptorHeap> dsvHeap;
	ThrowIfFailed(g_d3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&g_dsvHeap)));
}

/// 创建描述符。
/// 通过交换链获取后台缓冲区资源，然后创建RTV描述符。
void CreateRTV()
{
	// 获取RTV描述符堆的起始CPU描述符句柄
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(g_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	//ComPtr<ID3D12Resource> swapChainBuffer[2];
	// 获取后台缓冲区资源
	for (int i = 0; i < 2; i++)
	{
		//获得存于交换链中的后台缓冲区资源
		ThrowIfFailed(g_swapChain1->GetBuffer(i, IID_PPV_ARGS(&g_swapChainBuffer[0])));
		// 创建RTV描述符
		g_d3dDevice->CreateRenderTargetView(g_swapChainBuffer[0].Get(),
			nullptr,		//在交换链创建中已经定义了该资源的数据格式，所以这里指定为空指针
			rtvHeapHandle);	//描述符句柄结构体（这里是变体，继承自CD3DX12_CPU_DESCRIPTOR_HANDLE）
		//偏移到描述符堆中的下一个缓冲区
		rtvHeapHandle.Offset(1, g_rtvDescriptorSize);
	}
}
//// 先在CPU中创建好DS资源，然后通过CreateCommittedResource函数将DS资源提交至GPU显存中，最后创建DSV将显存中的DS资源和DSV句柄联系起来。
void CreateDSV()
{
	// 在CPU中创建好深度模板数据资源
	D3D12_RESOURCE_DESC dsvResourceDesc = { };
	dsvResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	// 资源维度
	dsvResourceDesc.Alignment = 0;	// 对齐方式
	dsvResourceDesc.Width = WINDOW_WIDTH;	// 资源宽度
	dsvResourceDesc.Height = WINDOW_HEIGHT;	// 资源高度
	dsvResourceDesc.DepthOrArraySize = 1;	// 资源深度或数组大小  纹理深度为1
	dsvResourceDesc.MipLevels = 1;	// 资源的最大层级 MIPMAP层级数量
	dsvResourceDesc.Format = DXGI_FORMAT_D32_FLOAT;	// 资源格式
	dsvResourceDesc.SampleDesc.Count = 4;	// 多重采样数量 1
	dsvResourceDesc.SampleDesc.Quality = g_msaaQualityLevels.NumQualityLevels - 1;	// 多重采样质量等级
	dsvResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;	// 资源布局 纹理布局（这里不指定）
	dsvResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;	// 深度模板资源的标志Flag
	dsvResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;	//24位深度，8位模板,还有个无类型的格式DXGI_FORMAT_R24G8_TYPELESS也可以使用

	// 创建深度模板数据资源
	D3D12_CLEAR_VALUE optClear = { };
	optClear.Format = DXGI_FORMAT_D32_FLOAT;	// 资源格式
	optClear.DepthStencil.Depth = 1.0f;	// 深度值
	optClear.DepthStencil.Stencil = 0;	// 模板值
	//创建一个资源和一个堆，并将资源提交至堆中（将深度模板数据提交至GPU显存中）
	//ComPtr<ID3D12Resource> depthStencilBuffer;
	auto l_heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);	// 堆类型为默认堆（不能写入）
	ThrowIfFailed(g_d3dDevice->CreateCommittedResource(
		&l_heapProperties,	// 堆类型为默认堆（不能写入）
		D3D12_HEAP_FLAG_NONE,	// 堆标志
		&dsvResourceDesc,		// 资源描述符 上面定义的DSV资源指针
		D3D12_RESOURCE_STATE_DEPTH_WRITE,	// 资源状态为初始状态
										// 不能将一个资源创建为 D3D12_RESOURCE_STATE_DEPTH_WRITE 状态，因为这个状态只能用于清除深度模板视图。
										// 需要将资源创建为 D3D12_RESOURCE_STATE_COMMON 状态，
										// 在使用之前转换到 D3D12_RESOURCE_STATE_DEPTH_WRITE 状态。
		&optClear,	// 清除值 上面定义的优化值指针
		IID_PPV_ARGS(g_depthStencilBuffer.GetAddressOf())	//返回深度模板资源
	));

	//创建DSV(必须填充DSV属性结构体，和创建RTV不同，RTV是通过句柄)
	//D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	//dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	//dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	//dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	//dsvDesc.Texture2D.MipSlice = 0;
	// 创建DSV描述符
	g_d3dDevice->CreateDepthStencilView(g_depthStencilBuffer.Get(),
		nullptr,		//D3D12_DEPTH_STENCIL_VIEW_DESC类型指针，可填&dsvDesc（见上注释代码），
		//由于在创建深度模板资源时已经定义深度模板数据属性，所以这里可以指定为空指针
		g_dsvHeap->GetCPUDescriptorHandleForHeapStart());	//DSV句柄
}

/// 9.资源转换。
/// 标记DS资源的状态
void TransitionDSResource()
{
	// 标记DS资源的状态
	ID3D12Resource* l_pResourc = g_depthStencilBuffer.Get();
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(l_pResourc,
		D3D12_RESOURCE_STATE_COMMON,		//转换前状态（创建时的状态，即CreateCommittedResource函数中定义的状态）
		D3D12_RESOURCE_STATE_DEPTH_WRITE);	//转换后状态为可写入的深度图，还有一个D3D12_RESOURCE_STATE_DEPTH_READ是只可读的深度图
	g_commandList->ResourceBarrier(1, 	//Barrier屏障个数
		&barrier);

	// 将命令从命令列表传入命令队列，也就是从CPU传入GPU的过程。注意：在传入命令队列前必须关闭命令列表。
	ThrowIfFailed(g_commandList->Close());	// 命令添加完后将其关闭
	ID3D12CommandList* cmdsLists[] = { g_commandList.Get() };	// 声明并定义命令列表数组
	g_commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);	// 将命令列表数组中的命令列表传入命令队列

}

/// 11.设置围栏刷新命令队列。
/// 12.将命令从列表传至队列。
void FlushCmdQueue()
{
	// 当前帧的Fence值加1
	g_currentFence++;	//CPU传完命令并关闭后，将当前围栏值+1

	// 将命令队列中的命令传入Fence中
	// 当GPU处理完CPU传入的命令后，将fence接口中的围栏值+1，即fence->GetCompletedValue()+1
	ThrowIfFailed(g_commandQueue->Signal(g_fence.Get(), g_currentFence));

	// 如果Fence的完成值小于当前围栏（帧）的Fence值，说明GPU还没有执行完当前帧的命令
	if (g_fence->GetCompletedValue() < g_currentFence)
	{
		// 创建一个事件句柄
		HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenceSetDone"); //CreateEventEx EVENT_ALL_ACCESS

		// 将Fence的完成值设置为当前帧的Fence值
		// 当围栏达到mCurrentFence值（即执行到Signal（）指令修改了围栏值）时触发的eventHandle事件
		ThrowIfFailed(g_fence->SetEventOnCompletion(g_currentFence, eventHandle));

		// 等待GPU命中围栏，激发事件（阻塞当前线程直到事件触发，注意此Enent需先设置再等待，
		// 如果没有Set就Wait，就死锁了，Set永远不会调用，所以也就没线程可以唤醒这个线程）
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
	// 设置视口
	g_viewPort.TopLeftX = 0;
	g_viewPort.TopLeftY = 0;
	g_viewPort.Width = static_cast<float>(WINDOW_WIDTH);
	g_viewPort.Height = static_cast<float>(WINDOW_HEIGHT);
	g_viewPort.MinDepth = 0.0f;
	g_viewPort.MaxDepth = 1.0f;

	// 裁剪矩形设置（矩形外的像素都将被剔除）
	// 前两个为左上点坐标，后两个为右下点坐标
	g_scissorRect.left = 0;
	g_scissorRect.top = 0;
	g_scissorRect.right = WINDOW_WIDTH;
	g_scissorRect.bottom = WINDOW_HEIGHT;
}

