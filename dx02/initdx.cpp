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

//AnsiToWString������ת���ɿ��ַ����͵��ַ�����wstring��
//��Windowsƽ̨�ϣ�����Ӧ�ö�ʹ��wstring��wchar_t������ʽ�����ַ���ǰ+L
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
	/*����D3D12���Բ�*/
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

/// �����豸
/// �豸����ͨ��DXGI API��IDXGIFactory�ӿ�������
void CreateDevice()
{
	//ComPtr<IDXGIFactory4> dxgiFactory;
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&g_dxgiFactory)));
	// ComPtr<ID3D12Device> d3dDevice;
	ThrowIfFailed(D3D12CreateDevice(
		nullptr,					//�˲����������Ϊnullptr����ʹ����������
		D3D_FEATURE_LEVEL_12_0,		//Ӧ�ó�����ҪӲ����֧�ֵ���͹��ܼ���
		IID_PPV_ARGS(&g_d3dDevice))	//���������豸
	);
}

/// ����Χ����ͬ��CPU��GPU��
/// Χ��������ͬ��CPU��GPU�ģ�CPU����ͨ��Χ�����ж�GPU�Ƿ������ĳЩ������
void CreateFence()
{
	// ComPtr<ID3D12Fence> fence;
	ThrowIfFailed(g_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)));
}

/// ��ȡ��������С��
/// ����������֪������������ÿ��Ԫ�صĴ�С���������ڲ�ͬ��GPUƽ̨�ϵĴ�С���죩��
/// ��������֮���ڵ�ַ����ƫ�����ҵ����е�������Ԫ�ء�/// �����ȡ������������С��
/// �ֱ���RTV����ȾĿ�껺��������������DSV�����ģ�建��������������
/// CBV_SRV_UAV����������������������ɫ����Դ������������������ʻ�������������
void GetDescriptorSize()
{
	g_rtvDescriptorSize = g_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	g_dsvDescriptorSize = g_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	g_cbv_srv_uavDescriptorSize = g_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

/// ����MSAA��������ԡ�
void SetMSAA()
{
	//D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQualityLevels;
	g_msaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	//UNORM�ǹ�һ��������޷�������
	g_msaaQualityLevels.SampleCount = 1;
	g_msaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	g_msaaQualityLevels.NumQualityLevels = 0;
	//��ǰͼ��������MSAA���ز�����֧�֣�ע�⣺�ڶ������������������������
	ThrowIfFailed(g_d3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &g_msaaQualityLevels, sizeof(g_msaaQualityLevels)));
	//NumQualityLevels��Check��������������
	//���֧��MSAA����Check�������ص�NumQualityLevels > 0
	//expressionΪ�٣���Ϊ0��������ֹ�������У�����ӡһ��������Ϣ
	assert(g_msaaQualityLevels.NumQualityLevels > 0);
}

/// ����������С������б������������
void CreateCommandObject()
{
	//�����������
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;	//ָ�������������
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;	//ָ��������е�����
	// ComPtr<ID3D12CommandQueue> cmdQueue;
	ThrowIfFailed(g_d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_commandQueue)));

	//�������������
	// ComPtr<ID3D12CommandAllocator> cmdAllocator;
	ThrowIfFailed(g_d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_commandAllocator)));

	//���������б�
	// ComPtr<ID3D12GraphicsCommandList> cmdList;
	// ע��˴��Ľӿ�����ID3D12GraphicsCommandList��������ID3D12CommandList
	ThrowIfFailed(g_d3dDevice->CreateCommandList(
		0,								// ����ֵΪ0����GPU
		D3D12_COMMAND_LIST_TYPE_DIRECT, // �����б�����
		g_commandAllocator.Get(), 		// ����������ӿ�ָ��
		nullptr, 						// ��ʼ����(��ˮ��)״̬����PSO�����ﲻ���ƣ����Կ�ָ��
		IID_PPV_ARGS(&g_commandList))	// ���ص������б�ӿ�ָ��
	);
	// �ر������б���Ϊ����ֻ��Ҫһ�Σ������ڴ������͹ر�
	g_commandList->Close();		// ���������б�ǰ���뽫��ر�
}

/// ������������
/// �������д�����ȾĿ����Դ������̨��������Դ��ͨ��DXGI API�µ�IDXGIFactory�ӿ���������������
/// ���ǽ���MSAA���ز�������Ϊ�����ñȽ��鷳������ֱ������MSAA���������count����Ϊ1������Ϊ0��
/// ��Ҫע�⣬CreateSwapChain�����ĵ�һ��������ʵ��������нӿ�ָ�룬�����豸�ӿ�ָ�룬�����������󵼡�
void CreateSwapChain()
{
	// ComPtr<IDXGISwapChain> swapChain;
	// �ͷ�֮ǰ�����Ľ�������Ȼ������ؽ�
	g_swapChain1.Reset();

	//DXGI_SWAP_CHAIN_DESC swapChainDesc = {};	// �������������ṹ��
	//swapChainDesc.BufferDesc.Width = WINDOW_WIDTH;			// �������ֱ��ʵĿ��
	//swapChainDesc.BufferDesc.Height = WINDOW_HEIGHT;		// �������ֱ��ʵĸ߶�
	//swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;	// ˢ���ʵķ���
	//swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;	// ˢ���ʵķ�ĸ
	//swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	// ����������ʾ��ʽ ���ø�ʽΪ RGBA ��ʽ
	//swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;	// ɨ����˳��  ����ɨ��VS����ɨ��(δָ����)
	//swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;	// ����ģʽ ͼ�������Ļ�����죨δָ���ģ�
	//swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// ��������; ��������Ⱦ����̨������������Ϊ��ȾĿ�꣩
	//swapChainDesc.SampleDesc.Count = 1;		// ���ز�������
	//swapChainDesc.SampleDesc.Quality = 0;	// ���ز��������ȼ�
	//swapChainDesc.BufferCount = 2;			// ����������
	//swapChainDesc.OutputWindow = g_hwnd;	// ���(��Ⱦ)���ھ��
	//swapChainDesc.Windowed = true;			// �Ƿ񴰿ڻ�
	//swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	// ����Ч��
	//swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	// ��������־ ����Ӧ����ģʽ���Զ�ѡ�������ڵ�ǰ���ڳߴ����ʾģʽ��

	//// ����������
	//ThrowIfFailed(g_dxgiFactory->CreateSwapChain(
	//	g_commandQueue.Get(),	// ������нӿ�ָ�� ����ΪNULL
	//	&swapChainDesc,			// ������������ ����ΪNULL
	//	g_swapChain.GetAddressOf()	// ���ڽ��ܽ�����
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

	// ����������
	ThrowIfFailed(g_dxgiFactory->CreateSwapChainForHwnd(
		g_commandQueue.Get(),	// ������нӿ�ָ�� ����ΪNULL
		g_hwnd,					// ���ھ�� ����ΪNULL
		&swapChainDesc,			// ������������ ����ΪNULL
		nullptr,				// ȫ��״̬�µĴ���״̬ ΪNULLʱʹ��Ĭ��ֵ
		nullptr,				// ȫ��״̬�µ���� ΪNULLʱʹ��Ĭ��ֵ
		g_swapChain1.GetAddressOf()	// ���ڽ��ܽ�����
	));
}

/// �����������ѡ�
/// �����������һ�������ڴ�ռ䡣��Ϊ��˫��̨���壬��������Ҫ�������2��RTV��RTV�ѣ�
/// �����ģ�建��ֻ��һ�������Դ���1��DSV��DSV�ѡ�
/// ����������������Խṹ�壬Ȼ��ͨ���豸�����������ѡ�
void CreateDescriptorHeap()
{
	// ����RTV��������
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = 2;		// ����������
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	// ����������
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// �������ѱ�־
	rtvHeapDesc.NodeMask = 0;	// �ڵ�����
	//ComPtr<ID3D12DescriptorHeap> rtvHeap;
	ThrowIfFailed(g_d3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&g_rtvHeap)));

	// ����DSV��������
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;		// ����������
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;	// ����������
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// �������ѱ�־
	dsvHeapDesc.NodeMask = 0;	// �ڵ�����
	//ComPtr<ID3D12DescriptorHeap> dsvHeap;
	ThrowIfFailed(g_d3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&g_dsvHeap)));
}

/// ������������
/// ͨ����������ȡ��̨��������Դ��Ȼ�󴴽�RTV��������
void CreateRTV()
{
	// ��ȡRTV�������ѵ���ʼCPU���������
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(g_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	//ComPtr<ID3D12Resource> swapChainBuffer[2];
	// ��ȡ��̨��������Դ
	for (int i = 0; i < 2; i++)
	{
		//��ô��ڽ������еĺ�̨��������Դ
		ThrowIfFailed(g_swapChain1->GetBuffer(i, IID_PPV_ARGS(&g_swapChainBuffer[0])));
		// ����RTV������
		g_d3dDevice->CreateRenderTargetView(g_swapChainBuffer[0].Get(),
			nullptr,		//�ڽ������������Ѿ������˸���Դ�����ݸ�ʽ����������ָ��Ϊ��ָ��
			rtvHeapHandle);	//����������ṹ�壨�����Ǳ��壬�̳���CD3DX12_CPU_DESCRIPTOR_HANDLE��
		//ƫ�Ƶ����������е���һ��������
		rtvHeapHandle.Offset(1, g_rtvDescriptorSize);
	}
}
//// ����CPU�д�����DS��Դ��Ȼ��ͨ��CreateCommittedResource������DS��Դ�ύ��GPU�Դ��У���󴴽�DSV���Դ��е�DS��Դ��DSV�����ϵ������
void CreateDSV()
{
	// ��CPU�д��������ģ��������Դ
	D3D12_RESOURCE_DESC dsvResourceDesc = { };
	dsvResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	// ��Դά��
	dsvResourceDesc.Alignment = 0;	// ���뷽ʽ
	dsvResourceDesc.Width = WINDOW_WIDTH;	// ��Դ���
	dsvResourceDesc.Height = WINDOW_HEIGHT;	// ��Դ�߶�
	dsvResourceDesc.DepthOrArraySize = 1;	// ��Դ��Ȼ������С  �������Ϊ1
	dsvResourceDesc.MipLevels = 1;	// ��Դ�����㼶 MIPMAP�㼶����
	dsvResourceDesc.Format = DXGI_FORMAT_D32_FLOAT;	// ��Դ��ʽ
	dsvResourceDesc.SampleDesc.Count = 4;	// ���ز������� 1
	dsvResourceDesc.SampleDesc.Quality = g_msaaQualityLevels.NumQualityLevels - 1;	// ���ز��������ȼ�
	dsvResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;	// ��Դ���� �����֣����ﲻָ����
	dsvResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;	// ���ģ����Դ�ı�־Flag
	dsvResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;	//24λ��ȣ�8λģ��,���и������͵ĸ�ʽDXGI_FORMAT_R24G8_TYPELESSҲ����ʹ��

	// �������ģ��������Դ
	D3D12_CLEAR_VALUE optClear = { };
	optClear.Format = DXGI_FORMAT_D32_FLOAT;	// ��Դ��ʽ
	optClear.DepthStencil.Depth = 1.0f;	// ���ֵ
	optClear.DepthStencil.Stencil = 0;	// ģ��ֵ
	//����һ����Դ��һ���ѣ�������Դ�ύ�����У������ģ�������ύ��GPU�Դ��У�
	//ComPtr<ID3D12Resource> depthStencilBuffer;
	auto l_heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);	// ������ΪĬ�϶ѣ�����д�룩
	ThrowIfFailed(g_d3dDevice->CreateCommittedResource(
		&l_heapProperties,	// ������ΪĬ�϶ѣ�����д�룩
		D3D12_HEAP_FLAG_NONE,	// �ѱ�־
		&dsvResourceDesc,		// ��Դ������ ���涨���DSV��Դָ��
		D3D12_RESOURCE_STATE_DEPTH_WRITE,	// ��Դ״̬Ϊ��ʼ״̬
										// ���ܽ�һ����Դ����Ϊ D3D12_RESOURCE_STATE_DEPTH_WRITE ״̬����Ϊ���״ֻ̬������������ģ����ͼ��
										// ��Ҫ����Դ����Ϊ D3D12_RESOURCE_STATE_COMMON ״̬��
										// ��ʹ��֮ǰת���� D3D12_RESOURCE_STATE_DEPTH_WRITE ״̬��
		&optClear,	// ���ֵ ���涨����Ż�ֵָ��
		IID_PPV_ARGS(g_depthStencilBuffer.GetAddressOf())	//�������ģ����Դ
	));

	//����DSV(�������DSV���Խṹ�壬�ʹ���RTV��ͬ��RTV��ͨ�����)
	//D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	//dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	//dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	//dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	//dsvDesc.Texture2D.MipSlice = 0;
	// ����DSV������
	g_d3dDevice->CreateDepthStencilView(g_depthStencilBuffer.Get(),
		nullptr,		//D3D12_DEPTH_STENCIL_VIEW_DESC����ָ�룬����&dsvDesc������ע�ʹ��룩��
		//�����ڴ������ģ����Դʱ�Ѿ��������ģ���������ԣ������������ָ��Ϊ��ָ��
		g_dsvHeap->GetCPUDescriptorHandleForHeapStart());	//DSV���
}

/// 9.��Դת����
/// ���DS��Դ��״̬
void TransitionDSResource()
{
	// ���DS��Դ��״̬
	ID3D12Resource* l_pResourc = g_depthStencilBuffer.Get();
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(l_pResourc,
		D3D12_RESOURCE_STATE_COMMON,		//ת��ǰ״̬������ʱ��״̬����CreateCommittedResource�����ж����״̬��
		D3D12_RESOURCE_STATE_DEPTH_WRITE);	//ת����״̬Ϊ��д������ͼ������һ��D3D12_RESOURCE_STATE_DEPTH_READ��ֻ�ɶ������ͼ
	g_commandList->ResourceBarrier(1, 	//Barrier���ϸ���
		&barrier);

	// ������������б���������У�Ҳ���Ǵ�CPU����GPU�Ĺ��̡�ע�⣺�ڴ����������ǰ����ر������б�
	ThrowIfFailed(g_commandList->Close());	// ������������ر�
	ID3D12CommandList* cmdsLists[] = { g_commandList.Get() };	// ���������������б�����
	g_commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);	// �������б������е������б����������

}

/// 11.����Χ��ˢ��������С�
/// 12.��������б������С�
void FlushCmdQueue()
{
	// ��ǰ֡��Fenceֵ��1
	g_currentFence++;	//CPU��������رպ󣬽���ǰΧ��ֵ+1

	// ����������е������Fence��
	// ��GPU������CPU���������󣬽�fence�ӿ��е�Χ��ֵ+1����fence->GetCompletedValue()+1
	ThrowIfFailed(g_commandQueue->Signal(g_fence.Get(), g_currentFence));

	// ���Fence�����ֵС�ڵ�ǰΧ����֡����Fenceֵ��˵��GPU��û��ִ���굱ǰ֡������
	if (g_fence->GetCompletedValue() < g_currentFence)
	{
		// ����һ���¼����
		HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenceSetDone"); //CreateEventEx EVENT_ALL_ACCESS

		// ��Fence�����ֵ����Ϊ��ǰ֡��Fenceֵ
		// ��Χ���ﵽmCurrentFenceֵ����ִ�е�Signal����ָ���޸���Χ��ֵ��ʱ������eventHandle�¼�
		ThrowIfFailed(g_fence->SetEventOnCompletion(g_currentFence, eventHandle));

		// �ȴ�GPU����Χ���������¼���������ǰ�߳�ֱ���¼�������ע���Enent���������ٵȴ���
		// ���û��Set��Wait���������ˣ�Set��Զ������ã�����Ҳ��û�߳̿��Ի�������̣߳�
		WaitForSingleObject(eventHandle, INFINITE);

		// �ر��¼����
		CloseHandle(eventHandle);
	}
}

/// 10.�����ӿںͲü����Ρ�
void CreateViewPortAndScissorRect()
{
	//D3D12_VIEWPORT viewPort;
	//D3D12_RECT scissorRect;
	// �����ӿ�
	g_viewPort.TopLeftX = 0;
	g_viewPort.TopLeftY = 0;
	g_viewPort.Width = static_cast<float>(WINDOW_WIDTH);
	g_viewPort.Height = static_cast<float>(WINDOW_HEIGHT);
	g_viewPort.MinDepth = 0.0f;
	g_viewPort.MaxDepth = 1.0f;

	// �ü��������ã�����������ض������޳���
	// ǰ����Ϊ���ϵ����꣬������Ϊ���µ�����
	g_scissorRect.left = 0;
	g_scissorRect.top = 0;
	g_scissorRect.right = WINDOW_WIDTH;
	g_scissorRect.bottom = WINDOW_HEIGHT;
}

