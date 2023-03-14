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

//���ÿ�lib
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")




//AnsiToWString������ת���ɿ��ַ����͵��ַ�����wstring��
//��Windowsƽ̨�ϣ�����Ӧ�ö�ʹ��wstring��wchar_t������ʽ�����ַ���ǰ+L
inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}
//DxException��
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
/// �궨��ThrowIfFailed
/// D3D12��������������᷵��HRESULT��ThrowIfFailed����Խ���HRESULTֵ���Ӷ��ж��Ƿ񴴽��ɹ���
/// �����չ�����Ǹ������׳��쳣�ĺ��������ܽ���Ӧ�ļ����к���ʾ������
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
//D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};	// ����Ҫ��ȫ�ֱ���
ComPtr<ID3D12CommandQueue> g_commandQueue;
ComPtr<ID3D12CommandAllocator> g_cmdAllocator;
ComPtr<ID3D12GraphicsCommandList> g_commandList;
// CreateSwapChain()
ComPtr<IDXGISwapChain> g_swapChain;
//DXGI_SWAP_CHAIN_DESC g_swapChainDesc;	// ����Ҫ��ȫ�ֱ���
// CreateDescriptorHeap()
//D3D12_DESCRIPTOR_HEAP_DESC g_rtvDescriptorHeapDesc;	// ����Ҫ��ȫ�ֱ���
ComPtr<ID3D12DescriptorHeap> g_rtvHeap;
//D3D12_DESCRIPTOR_HEAP_DESC g_dsvDescriptorHeapDesc;	// ����Ҫ��ȫ�ֱ���
ComPtr<ID3D12DescriptorHeap> g_dsvHeap;
// CreateRTV()
//CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle;
ComPtr<ID3D12Resource> g_swapChainBuffer[2];
// CreateDSV()
D3D12_RESOURCE_DESC dsvResourceDesc;
ComPtr<ID3D12Resource> depthStencilBuffer;
// FlushCmdQueue()
int mCurrentFence = 0;	//��ʼCPU�ϵ�Χ����Ϊ0
// CreateViewPortAndScissorRect()
D3D12_VIEWPORT g_viewPort;
D3D12_RECT g_scissorRect;
// Draw()
UINT g_mCurrentBackBuffer = 0;
//���������ھ��
HWND g_hwnd = 0;	//ĳ�����ڵľ����ShowWindow��UpdateWindow������Ҫ���ô˾��


//bool InitWindow(HINSTANCE hInstance, int nShowCmd);
void Draw();
int Run();
bool Init(HINSTANCE hInstance, int nShowCmd);
//���ڹ��̺���������,HWND�������ھ��
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lparam);

/// ��дWinMain���������������൱��C++�е�Main��ں���������������������һ�����Գ���Ĺ̶�д����
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

/// -----------------------------------����Ϣ��������----------------------------------
/// <summary>
/// ��Ϣ������ ���ص�����������ϵͳ�����ã�
/// �����յ�����Ϣ���ɸ����ڹ���
/// </summary>
/// <param name="hwnd"></param>
/// <param name="message">��Ϣ����</param>
/// <param name="wParam">��Ϣ����</param>
/// <param name="lParam">��Ϣ����</param>
/// <returns></returns>
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//��Ϣ����
	switch (message)
	{
		//�����ڱ�����ʱ����ֹ��Ϣѭ��
	case WM_DESTROY:
		PostQuitMessage(0);	//��ֹ��Ϣѭ����������WM_QUIT��Ϣ
		break;
	default:
		//������û�д������Ϣת����Ĭ�ϵĴ��ڹ���
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}


//0.����D3D12���Բ㡣���ڳ�ʼ��D3D�Ŀ�ͷ��
//�����豸��			CreateDevice()
//����Χ����			CreateFence()
//��ȡ��������С��	GetDescriptorSize()
//����MSAA��������ԡ�
//����������С������б������������
//������������
//�����������ѡ�
//������������
//��Դת����
//�����ӿںͲü����Ρ�
//����Χ��ˢ��������С�
//��������б������С�


/// 1.�����豸
/// ���ǽ�����ģ���װ�ɺ����������Ǵ����豸��
/// �����˽���DXGI API�����DXGI�Ļ���������ʹ�ö���ͼ��API�������еĵײ������ܽ���һ��ͨ��API�����д���
/// �����ǵ��豸����ͨ��DXGI API��IDXGIFactory�ӿ��������ġ�����������ҪCreateDXGIFactory��
/// Ȼ����CreateDevice��
void CreateDevice()
{
	//ComPtr<IDXGIFactory4> dxgiFactory;
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&g_dxgiFactory)));
	//ComPtr<ID3D12Device> g_d3dDevice;
	// 
	// ����D3D12.1���豸
	ThrowIfFailed(D3D12CreateDevice(
		nullptr,                    //�˲����������Ϊnullptr����ʹ����������
		D3D_FEATURE_LEVEL_12_1,		//Ӧ�ó�����ҪӲ����֧�ֵ���͹��ܼ���
		IID_PPV_ARGS(&g_d3dDevice)));	//���������豸
}


/// 2.ͨ���豸����Χ��fence���Ա�֮��ͬ��CPU��GPU��
/// CPU����ͨ��Χ�����ж�GPU�Ƿ������ĳЩ������
void CreateFence()
{
	//ComPtr<ID3D12Fence> fence;
	ThrowIfFailed(g_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
}

/// 3.��ȡ��������С
/// ����������֪������������ÿ��Ԫ�صĴ�С���������ڲ�ͬ��GPUƽ̨�ϵĴ�С���죩��
/// ��������֮���ڵ�ַ����ƫ�����ҵ����е�������Ԫ�ء�
/// �������ǻ�ȡ������������С���ֱ���
///  RTV����ȾĿ�껺��������������
///  DSV�����ģ�建��������������
///  CBV_SRV_UAV����������������������ɫ����Դ������������������ʻ�������������
void GetDescriptorSize()
{
	//UINT 
	g_rtvDescriptorSize = g_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//UINT 
	g_dsvDescriptorSize = g_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	//UINT 
	g_cbv_srv_uavDescriptorSize = g_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

/// 4.����MSAA��������ԡ�
/// �������ز������Խṹ�壬Ȼ��ͨ��CheckFeatureSupport��������NumQualityLevels��
/// ע�⣺�˴���ʹ��MSAA��������������Ϊ0��
void SetMSAA()
{
	//D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS g_msaaQualityLevels;
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

/// 5.����������С�����������������б�
/// �������ߵĹ�ϵ�ǣ�
///  ����CPU���������б�
///  Ȼ�󽫹���������������ϵ�����������б�
///  ��������б���������и�GPU������һ��ֻ���������ߵĴ���������
/// ע�⣬�����ڳ�ʼ��D3D12_COMMAND_QUEUE_DESCʱ��
/// ֻ��ʼ��������������������ڴ�������Ĭ�ϳ�ʼ���ˡ�
void CreateCommandObject()
{
	//�����������
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;	//ָ�������������
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;	//ָ��������е�����
	//ComPtr<ID3D12CommandQueue> g_commandQueue;
	ThrowIfFailed(g_d3dDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&g_commandQueue)));

	//���������б������
	//ComPtr<ID3D12CommandAllocator> g_cmdAllocator;
	ThrowIfFailed(g_d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_cmdAllocator)));//&cmdAllocator�ȼ���cmdAllocator.GetAddressOf

	//���������б�
	//ComPtr<ID3D12GraphicsCommandList> g_commandList;	
	//ע��˴��Ľӿ�����ID3D12GraphicsCommandList��������ID3D12CommandList
	ThrowIfFailed(g_d3dDevice->CreateCommandList(
		0,								//����ֵΪ0����GPU
		D3D12_COMMAND_LIST_TYPE_DIRECT, //�����б�����
		g_cmdAllocator.Get(),			//����������ӿ�ָ��
		nullptr,						//��ʼ����(��ˮ��)״̬����PSO�����ﲻ���ƣ����Կ�ָ��
		IID_PPV_ARGS(&g_commandList)));	//���ش����������б�

	// �ر������б���Ϊ����ֻ��Ҫһ�Σ������ڴ������͹ر�
	g_commandList->Close();				//���������б�ǰ���뽫��ر�
}

/// 6.����������
/// �������д�����ȾĿ����Դ������̨��������Դ��ͨ��DXGI API�µ�IDXGIFactory�ӿ���������������
/// ���ﻹ�ǽ���MSAA���ز�������Ϊ�����ñȽ��鷳������ֱ������MSAA���������count����Ϊ1������Ϊ0��
/// ��Ҫע��һ�㣬CreateSwapChain�����ĵ�һ��������ʵ��������нӿ�ָ�룬�����豸�ӿ�ָ�룬�����������󵼡�
void CreateSwapChain()
{
	//ComPtr<IDXGISwapChain> swapChain;
	// �ͷ�֮ǰ�����Ľ�������Ȼ������ؽ�
	g_swapChain.Reset();

	DXGI_SWAP_CHAIN_DESC swapChainDesc;	// �����������ṹ��
	swapChainDesc.BufferDesc.Width = 1280;	// �������ֱ��ʵĿ��
	swapChainDesc.BufferDesc.Height = 720;	// �������ֱ��ʵĸ߶�
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	//����������ʾ��ʽ ���ø�ʽΪ RGBA ��ʽ
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;	// ˢ���ʵķ���
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;	// ˢ���ʵķ�ĸ
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;	// ɨ����˳��  ����ɨ��VS����ɨ��(δָ����)
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;	// ����ģʽ ͼ�������Ļ�����죨δָ���ģ�
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// ��������; ��������Ⱦ����̨������������Ϊ��ȾĿ�꣩
	swapChainDesc.OutputWindow = g_hwnd;	// ��Ⱦ(���)���ھ��
	swapChainDesc.SampleDesc.Count = 1;		// ���ز�������
	swapChainDesc.SampleDesc.Quality = 0;	// ���ز�������
	swapChainDesc.Windowed = true;			// �Ƿ񴰿ڻ�
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	// ����Ч�� �̶�д��
	swapChainDesc.BufferCount = 2;			// ��̨������������˫���壩
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	// ��������־ ����Ӧ����ģʽ���Զ�ѡ�������ڵ�ǰ���ڳߴ����ʾģʽ��

	// ����DXGI�ӿ��µĹ����� ����������
	ThrowIfFailed(g_dxgiFactory->CreateSwapChain(
		g_commandQueue.Get(), 		// ������нӿ�ָ�� ����ΪNULL
		&swapChainDesc, 			// ������������ ����ΪNULL
		g_swapChain.GetAddressOf()));	// ���ڽ��ܽ�����

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

	//// ����������
	//ThrowIfFailed(g_dxgiFactory->CreateSwapChainForHwnd(
	//	g_commandQueue.Get(),	// ������нӿ�ָ�� ����ΪNULL
	//	g_hwnd,					// ���ھ�� ����ΪNULL
	//	&swapChainDesc,			// ������������ ����ΪNULL
	//	nullptr,				// ȫ��״̬�µĴ���״̬ ΪNULLʱʹ��Ĭ��ֵ
	//	nullptr,				// ȫ��״̬�µ���� ΪNULLʱʹ��Ĭ��ֵ
	//	&g_swapChain1			// ���ڽ��ܽ�����   g_swapChain1.GetAddressOf()
	//));
}


/// �����������ѣ�descriptorHeap����
/// ���������Ǵ����������һ�������ڴ�ռ䡣��Ϊ��˫��̨���壬����Ҫ�������2��RTV��RTV�ѣ�
/// �����ģ�建��ֻ��һ�������Դ���1��DSV��DSV�ѡ�
/// �������: ����������������Խṹ�壬Ȼ��ͨ���豸�����������ѡ�
void CreateDescriptorHeap()
{
	//���ȴ���RTV��
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc;
	rtvDescriptorHeapDesc.NumDescriptors = 2;		// ����������
	rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// �������ѱ�־
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	// ������������
	rtvDescriptorHeapDesc.NodeMask = 0;	// �ڵ�����
	//ComPtr<ID3D12DescriptorHeap> g_rtvHeap;
	ThrowIfFailed(g_d3dDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&g_rtvHeap)));

	//Ȼ�󴴽�DSV��
	D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc;
	dsvDescriptorHeapDesc.NumDescriptors = 1;		// ����������
	dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// �������ѱ�־
	dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;	// ������������
	dsvDescriptorHeapDesc.NodeMask = 0;	// �ڵ�����
	//ComPtr<ID3D12DescriptorHeap> g_dsvHeap;
	ThrowIfFailed(g_d3dDevice->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(&g_dsvHeap)));
}

/// �������е�������
/// �ȴ���RTV����������ǣ�
///  �ȴ�RTV�����õ��׸�RTV�����
///  Ȼ���ô��ڽ������е�RT��Դ��
///  ��󴴽�RTV��RT��Դ��RTV�����ϵ������
///  ����������RTV��С�����ڶ��еĵ�ַƫ�ơ�
///ע�������õ���CD3DX12_CPU_DESCRIPTOR_HANDL��
///���������d3dx12.hͷ�ļ��ж��壬DX�Ⲣû�м��ɣ�
///��Ҫ�����������ء������CD3DX12_CPU_DESCRIPTOR_HANDLE�Ǹ������࣬
///���Ĺ��캯����ʼ����D3D12_CPU_DESCRIPTOR_HANDLE�ṹ���е�Ԫ�أ�
///����ֱ�ӵ����乹�캯�����ɡ�
///֮�����ǻᾭ���õ�CD3DX12��ͷ�ı��������򻯳�ʼ���Ĵ�����д��
///�÷������û���һ�¡�
void CreateRTV()
{
	// ��ȡRTV�������ѵ���ʼCPU���������
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(g_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	//ComPtr<ID3D12Resource> g_swapChainBuffer[2];
	// ��ȡ��̨��������Դ
	for (int i = 0; i < 2; i++)
	{
		//��ô��ڽ������еĺ�̨��������Դ
		g_swapChain->GetBuffer(i, IID_PPV_ARGS(g_swapChainBuffer[i].GetAddressOf()));
		//����RTV������
		g_d3dDevice->CreateRenderTargetView(g_swapChainBuffer[i].Get(),
			nullptr,	//�ڽ������������Ѿ������˸���Դ�����ݸ�ʽ����������ָ��Ϊ��ָ��
			rtvHeapHandle);	//����������ṹ�壨�����Ǳ��壬�̳���CD3DX12_CPU_DESCRIPTOR_HANDLE��
		//ƫ�Ƶ����������е���һ��������
		rtvHeapHandle.Offset(1, g_rtvDescriptorSize);
	}
}


/// ����DSV
/// �������:
///  ����CPU�д�����DS��Դ��
///  Ȼ��ͨ��CreateCommittedResource������DS��Դ�ύ��GPU�Դ��У�
///  ��󴴽�DSV���Դ��е�DS��Դ��DSV�����ϵ������
void CreateDSV()
{
	//��CPU�д��������ģ��������Դ
	//D3D12_RESOURCE_DESC dsvResourceDesc;
	dsvResourceDesc.Alignment = 0;	//ָ������
	dsvResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	//ָ����Դά�ȣ����ͣ�ΪTEXTURE2D
	dsvResourceDesc.DepthOrArraySize = 1;	//  ��Դ��Ȼ������С  �������Ϊ1
	dsvResourceDesc.Width = WINDOW_WIDTH;	// ��Դ��
	dsvResourceDesc.Height = WINDOW_HEIGHT;	// ��Դ��
	dsvResourceDesc.MipLevels = 1;			// ��Դ�����㼶 MIPMAP�㼶����
	dsvResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;	//ָ�������֣����ﲻָ����
	dsvResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;	//���ģ����Դ��Flag
	dsvResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;	//24λ��ȣ�8λģ��,���и������͵ĸ�ʽDXGI_FORMAT_R24G8_TYPELESSҲ����ʹ��
	dsvResourceDesc.SampleDesc.Count = 1;	//���ز�������  �����ֵ��swapChainDesc ��Ӧ��
	dsvResourceDesc.SampleDesc.Quality = g_msaaQualityLevels.NumQualityLevels - 1;	//���ز��������ȼ�

	// �������ģ��������Դ
	CD3DX12_CLEAR_VALUE optClear = {};	// �����Դ���Ż�ֵ��������������ִ���ٶȣ�CreateCommittedResource�����д��룩
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//24λ��ȣ�8λģ��,���и������͵ĸ�ʽDXGI_FORMAT_R24G8_TYPELESSҲ����ʹ��
	optClear.DepthStencil.Depth = 1;	//��ʼ���ֵΪ 1.0f
	optClear.DepthStencil.Stencil = 0;	//��ʼģ��ֵΪ0
	//����һ����Դ��һ���ѣ�������Դ�ύ�����У������ģ�������ύ��GPU�Դ��У�
	//ComPtr<ID3D12Resource> depthStencilBuffer;
	auto l_heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(g_d3dDevice->CreateCommittedResource(
		&l_heapProperties,			// ������ΪĬ�϶ѣ�����д�룩D3D12_HEAP_TYPE_UPLOAD
		D3D12_HEAP_FLAG_NONE,		//Flag
		&dsvResourceDesc,			//���涨���DSV��Դָ��
		D3D12_RESOURCE_STATE_COMMON,//��Դ��״̬Ϊ��ʼ״̬
		&optClear,					//���涨����Ż�ֵָ��
		IID_PPV_ARGS(&depthStencilBuffer)));	//�������ģ����Դ

	//����DSV(�������DSV���Խṹ�壬�ʹ���RTV��ͬ��RTV��ͨ�����)
		//D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		//dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		//dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		//dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		//dsvDesc.Texture2D.MipSlice = 0;
	/// ����DSV������
	g_d3dDevice->CreateDepthStencilView(depthStencilBuffer.Get(),
		nullptr,	//D3D12_DEPTH_STENCIL_VIEW_DESC����ָ�룬����&dsvDesc������ע�ʹ��룩��
		//�����ڴ������ģ����Դʱ�Ѿ��������ģ���������ԣ������������ָ��Ϊ��ָ��
		g_dsvHeap->GetCPUDescriptorHandleForHeapStart());	//DSV���
}

/// ���DS��Դ��״̬��
/// ��Ϊ��Դ�ڲ�ͬ��ʱ������Ų�ͬ�����ã�
/// ���磬��ʱ������ֻ������ʱ�����ǿ�д��ġ�
/// ������ResourceBarrier�µ�Transition������ת����Դ״̬��
void ResourceBarrierBuild()
{
	// ���DS��Դ��״̬
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(depthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON,		//ת��ǰ״̬������ʱ��״̬����CreateCommittedResource�����ж����״̬��
		D3D12_RESOURCE_STATE_DEPTH_WRITE);	//ת����״̬Ϊ��д������ͼ������һ��D3D12_RESOURCE_STATE_DEPTH_READ��ֻ�ɶ������ͼ
	g_commandList->ResourceBarrier(1,		//Barrier���ϸ���
		&barrier);

	//�������������cmdList�󣬻���Ҫ��ExecuteCommandLists������
	//������������б���������У�Ҳ���Ǵ�CPU����GPU�Ĺ��̡�
	//ע�⣺�ڴ����������ǰ����ر������б�
	ThrowIfFailed(g_commandList->Close());	//������������ر�
	ID3D12CommandList* cmdLists[] = { g_commandList.Get() };	//���������������б�����
	g_commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);	//������������б����������

}

//Ϊ��ʹCPU��GPUͬ��������֮ǰ�Ѿ�������Χ���ӿڣ�������Ҫʵ��Χ�����롣
//ʵ��˼���ǣ�Χ����ʼֵΪ0�������ΪGPU�˵�Χ��ֵ����
//�����Ƕ���һ����ǰΧ��ֵҲΪ0�������ΪCPU�˵�Χ��ֵ����
//��CPU���������GPU�󣬵�ǰΧ��ֵ++��CPUΧ��++����
//����GPU������CPU�����������Χ��ֵ++��GPUΧ��++����
//Ȼ���ж�Χ��ֵ��CPUΧ��ֵ���͵�ǰΧ��ֵ��GPUΧ��ֵ���Ĵ�С����ȷ��GPU�Ƿ�����Χ���㣬
//���û�����У���ȴ����к󴥷��¼���

/// 11.����Χ��ˢ��������С�
/// 12.��������б������С�
void FlushCmdQueue()
{
	mCurrentFence++;	//CPU��������رպ󣬽���ǰΧ��ֵ+1

	// ����������е������Fence��
	//��GPU������CPU���������󣬽�fence�ӿ��е�Χ��ֵ+1����fence->GetCompletedValue()+1
	ThrowIfFailed(g_commandQueue->Signal(fence.Get(), mCurrentFence));

	// ���Fence�����ֵС�ڵ�ǰΧ����֡����Fenceֵ��˵��GPU��û��ִ���굱ǰ֡������
	if (fence->GetCompletedValue() < mCurrentFence)	//���С�ڣ�˵��GPUû�д�������������
	{
		// ����һ���¼����
		HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenceSetDone");

		// ��Fence�����ֵ����Ϊ��ǰ֡��Fenceֵ
		// ��Χ���ﵽmCurrentFenceֵ����ִ�е�Signal����ָ���޸���Χ��ֵ��ʱ������eventHandle�¼�
		ThrowIfFailed(fence->SetEventOnCompletion(mCurrentFence, eventHandle));

		//�ȴ�GPU����Χ���������¼���������ǰ�߳�ֱ���¼�������ע���Enent���������ٵȴ���
		//���û��Set��Wait���������ˣ�Set��Զ������ã�����Ҳ��û�߳̿��Ի�������̣߳�
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
	//�ӿ�����
	g_viewPort.TopLeftX = 0;
	g_viewPort.TopLeftY = 0;
	g_viewPort.Width = static_cast<float>(WINDOW_WIDTH);
	g_viewPort.Height = static_cast<float>(WINDOW_HEIGHT);
	g_viewPort.MaxDepth = 1.0f;
	g_viewPort.MinDepth = 0.0f;

	//�ü��������ã�����������ض������޳���
	//ǰ����Ϊ���ϵ����꣬������Ϊ���µ�����
	g_scissorRect.left = 0;
	g_scissorRect.top = 0;
	g_scissorRect.right = WINDOW_WIDTH;
	g_scissorRect.bottom = WINDOW_HEIGHT;
}

/// ���Ƕ�֮ǰ�Ĵ�����΢�����װ������Ϊ�������߼�����⣬����һ���ӷ�װ�ĺ��
/// ���ȣ����ǽ�һ��InitWindow������
/// Ȼ��֮ǰд�Ĵ������ڴ��뿽����ȥ��
/// �˺�������һ������ֵ��������ڴ����ɹ��򷵻�true��
bool InitWindow(HINSTANCE hInstance, int nShowCmd)
{
	// ���ڳ�ʼ�������ṹ��(WNDCLASS)
	WNDCLASSEX wndClass = {};
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;	// ���ڷ��  ����������߸ı䣬�����»��ƴ���
	wndClass.lpfnWndProc = MainWndProc;			// ��Ϣ�������ص�	Ĭ���� DefWindowProc Ĭ�ϵ���Ϣ���� default Window procedure
	wndClass.cbClsExtra = 0;	// �����������ֶ���Ϊ��ǰӦ�÷��������ڴ�ռ䣨���ﲻ���䣬������0��
	wndClass.cbWndExtra = 0;	// �����������ֶ���Ϊ��ǰӦ�÷��������ڴ�ռ䣨���ﲻ���䣬������0��
	wndClass.hInstance = hInstance;	// Ӧ�ó���ʵ���������WinMain���룩
	wndClass.hIcon = LoadIcon(0, IDC_ARROW);	// ����ͼ�� // ʹ��Ĭ�ϵ�Ӧ�ó���ͼ��
	wndClass.hCursor = LoadCursor(0, IDC_ARROW);// ���ڹ��// ʹ�ñ�׼�����ָ����ʽ
	wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);	//ָ���˰�ɫ������ˢ��� ��ֹ���ĵı����ػ�
	wndClass.lpszMenuName = 0;				// û�в˵���
	wndClass.lpszClassName = L"MainWnd";	// ��������
	//������ע��ʧ��
	if (!RegisterClassEx(&wndClass))
	{
		//��Ϣ����������1����Ϣ���������ھ������ΪNULL������2����Ϣ����ʾ���ı���Ϣ������3�������ı�������4����Ϣ����ʽ
		MessageBox(0, L"RegisterClass Failed", 0, 0);
		return 0;
	}

	RECT R;	//�ü�����
	R.left = 0;
	R.top = 0;
	R.right = WINDOW_WIDTH;
	R.bottom = WINDOW_HEIGHT;
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);	//���ݴ��ڵĿͻ�����С���㴰�ڵĴ�С
	int width = R.right - R.left;
	int hight = R.bottom - R.top;

	//��������,���ز���ֵ
	//CreateWindowW(lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam)
	g_hwnd = CreateWindowExW(
		WS_EX_OVERLAPPEDWINDOW,			// ��չ������ʽ
		L"MainWnd", L"DX12Initialize", 	// �������� & ���ڱ���
		WS_OVERLAPPEDWINDOW, 			// Ҫ�����Ĵ�������
		CW_USEDEFAULT, CW_USEDEFAULT,	// ��ʼλ��(x, y)������븸���ڣ�
		width, hight,					// ��ʼ��С����ȣ����ȣ�
		NULL,							// �˴��ڵĸ����������ھ����
		NULL,							// �˵������
		hInstance,						// ����ľ����WinMain �ĵ�һ��������
		NULL							// �������ݡ���һ�㲻�ã����ݸ����ڹ��̵�ֵ
	);
	//p���ڴ���ʧ��
	if (!g_hwnd)
	{
		MessageBox(0, L"CreatWindow Failed", 0, 0);
		return 0;
	}

	// �����ƶ�(�Ǳ���)
	//MoveWindow(g_hwnd, 250, 80, WINDOW_WIDTH, WINDOW_HEIGHT, true);
	// ��ʾ���ڣ� g_nCmdShow��ʾ������
	ShowWindow(g_hwnd, nShowCmd);
	// ���´�������
	UpdateWindow(g_hwnd);

	return true;
}

//Draw������Ҫ�ǽ����ǵĸ�����Դ���õ���Ⱦ��ˮ����, 
//�����շ���������������������������cmdAllocator�������б�cmdList��
//Ŀ��������������б���������ڴ档
void Draw()
{
	/// �����������������cmdAllocator�������б�cmdList��Ŀ��������������б���������ڴ档
	ThrowIfFailed(g_cmdAllocator->Reset());		//�ظ�ʹ�ü�¼���������ڴ�
	ThrowIfFailed(g_commandList->Reset(g_cmdAllocator.Get(), nullptr));	//���������б����ڴ�
	// ����̨������Դ�ӳ���״̬ת������ȾĿ��״̬����׼������ͼ����Ⱦ��
	UINT& ref_mCurrentBackBuffer = g_mCurrentBackBuffer;
	auto l_barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(
		g_swapChainBuffer[ref_mCurrentBackBuffer].Get(),  //ת����ԴΪ��̨��������Դ
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);  //�ӳ��ֵ���ȾĿ��ת��
	g_commandList->ResourceBarrier(1, &l_barrier1);

	//�����������ӿںͲü����Ρ�
	g_commandList->RSSetViewports(1, &g_viewPort);
	g_commandList->RSSetScissorRects(1, &g_scissorRect);


	// �����̨����������Ȼ�����������ֵ��
	//�������Ȼ�ö������������������ַ����
	//��ͨ��ClearRenderTargetView������ClearDepthStencilView����������͸�ֵ��
	//�������ǽ�RT��Դ����ɫ��ֵΪBlack����ɫ��DarkRed�����죩��
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHeap->GetCPUDescriptorHandleForHeapStart(), ref_mCurrentBackBuffer, g_rtvDescriptorSize);
	g_commandList->ClearRenderTargetView(
		rtvHandle,
		DirectX::Colors::Black,
		0,
		nullptr);	//���RT����ɫΪ��ɫ�����Ҳ����òü�����
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = g_dsvHeap->GetCPUDescriptorHandleForHeapStart();
	g_commandList->ClearDepthStencilView(
		dsvHandle,	//DSV���������
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,	//FLAG
		1.0f,	//Ĭ�����ֵ
		0,	//Ĭ��ģ��ֵ
		0,	//�ü���������
		nullptr);	//�ü�����ָ��

	//Ȼ������ָ����Ҫ��Ⱦ�Ļ���������ָ��RTV��DSV��
	g_commandList->OMSetRenderTargets(1,//���󶨵�RTV����
		&rtvHandle,	//ָ��RTV�����ָ��
		true,	//RTV�����ڶ��ڴ�����������ŵ�
		&dsvHandle);	//ָ��DSV��ָ��

	//�ȵ���Ⱦ��ɣ�����Ҫ����̨��������״̬�ĳɳ���״̬��ʹ��֮���Ƶ�ǰ̨��������ʾ��
	//���ˣ��ر������б��ȴ�����������С�
	auto l_barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
		g_swapChainBuffer[ref_mCurrentBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	g_commandList->ResourceBarrier(1, &l_barrier2);//����ȾĿ�굽����
	//�������ļ�¼�ر������б�
	ThrowIfFailed(g_commandList->Close());

	//��CPU�����׼���ú���Ҫ����ִ�е������б����GPU��������С�
	//ʹ�õ���ExecuteCommandLists������
	ID3D12CommandList* commandLists[] = { g_commandList.Get() };//���������������б�����
	g_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);//������������б����������

	//Ȼ�󽻻�ǰ��̨������������������㷨��1��0��0��1��Ϊ���ú�̨������������ԶΪ0����
	ThrowIfFailed(g_swapChain->Present(0, 0));
	ref_mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;

	//�������Χ��ֵ��ˢ��������У�ʹCPU��GPUͬ������δ����ڵ�һƪ������ϸ���ͣ�����ֱ�ӷ�װ��
	FlushCmdQueue();


}

//Ȼ�������½�һ��Run��������֮ǰ����Ϣѭ�����븴�ƽ�ȥ��
//������ṹ����������
//��Ϊ���ǵ���������Ϸ����
//���Ե����˷�֧�ṹ��
//���û����Ϣ������ִ����Ϸ������߼��ļ��㣬
//�����Draw����֮��ᶨ��
//��Draw����ÿ����һ�Σ���ʵ����һ֡�����Ժ��ڻ�����ǰ�����֡ʱ��ļ��㣩��
int Run()
{
	//��Ϣѭ��
	//������Ϣ�ṹ��
	MSG msg = { 0 };
	//���GetMessage����������0��˵��û�н��ܵ�WM_QUIT
	while (msg.message != WM_QUIT)
	{
		//����д�����Ϣ�ͽ��д���
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) //PeekMessage�������Զ����msg�ṹ��Ԫ��
		{
			TranslateMessage(&msg);	//���̰���ת�������������Ϣת��Ϊ�ַ���Ϣ
			DispatchMessage(&msg);	//����Ϣ���ɸ���Ӧ�Ĵ��ڹ���
		}
		//�����ִ�ж�������Ϸ�߼�
		else
		{
			Draw();
		}
	}
	return (int)msg.wParam;
}

//���������ǽ�һ��InitDirect3D������
//��֮ǰ�����г��ĳ�ʼ�����裬
//���������������������Draw������ִ�У���
//�������������У������֮ǰ�Ĳ��趼���˺�����װ����
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

//���������ǽ�һ���ܵ�Init������
//��InitWindow��InitDirect3D��һ����һ���жϣ�
//���������ʼ��������ִ�У�
//���ж��ܵĳ�ʼ������ִ�У�����true��
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
