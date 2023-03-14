#pragma once
#include "dx02.h"
// Windows 头文件

#include "../common/d3dx12.h"
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
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

using namespace Microsoft::WRL;

extern ComPtr<IDXGIFactory4> g_dxgiFactory;
extern ComPtr<ID3D12Device> g_d3dDevice;
extern ComPtr<ID3D12Fence> g_fence;
extern ComPtr<ID3D12CommandQueue> g_commandQueue;
extern ComPtr<ID3D12CommandAllocator> g_commandAllocator;
extern ComPtr<ID3D12GraphicsCommandList> g_commandList;
extern ComPtr<IDXGISwapChain1> g_swapChain1;
extern ComPtr<ID3D12DescriptorHeap> g_rtvHeap;
extern ComPtr<ID3D12DescriptorHeap> g_dsvHeap;
extern ComPtr<ID3D12Resource> g_swapChainBuffer[2];
extern ComPtr<ID3D12Resource> g_depthStencilBuffer;

extern D3D12_VIEWPORT g_viewPort;
extern D3D12_RECT g_scissorRect;
extern UINT g_rtvDescriptorSize;
extern UINT g_dsvDescriptorSize;
extern UINT g_cbv_srv_uavDescriptorSize;
extern UINT g_mCurrentBackBuffer;

bool InitDirect3D();
void CreateDevice();
void CreateFence();
void GetDescriptorSize();
void SetMSAA();
void CreateCommandObject();
void CreateSwapChain();
void CreateDescriptorHeap();
void CreateRTV();
void CreateDSV();
void TransitionDSResource();
void FlushCmdQueue();
void CreateViewPortAndScissorRect();

inline std::wstring AnsiToWString(const std::string& str);

/// DxException类
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
#endif


