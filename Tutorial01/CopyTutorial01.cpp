#include <windows.h>	// ������ API�� �ʿ��� �Լ� �� Ÿ���� ����
#include <d3d11.h>		// Direct3D 11�� ���õ� ��� ����� ����
//#include <d3dx11.h>   // Direct3D ����� ���̺귯����, �ؽ�ó �ε�� ���� �۾��� �����ϰ� ���� (�� ���̺귯���� �� �̻� ���������� �������� ������ ���� �ڵ忡�� ���� ���)
#include "resource.h"

HINSTANCE				g_hInst = NULL;							    // ���� ���ø����̼��� �ν��Ͻ� �ڵ�, ������ â�� �����ϰų� �ٸ� WIn32 APIȣ�⿡�� ���
HWND					g_hWnd = NULL;							    // ������ ������ â�� �ڵ�
D3D_DRIVER_TYPE			g_driverType = D3D_DRIVER_TYPE_NULL;	    // Direct3D�� ����̹� Ÿ��. ��) �ϵ���� ����̹�, ����Ʈ���� ����̹�
D3D_FEATURE_LEVEL		g_featureLevel = D3D_FEATURE_LEVEL_11_0;    // Direct3D���� �����ϴ� ��� ����
ID3D11Device*			g_pd3dDevice = NULL;					    // Direct3D 11 ��ġ�� ��Ÿ���� �������̽�. GPU�� ���� ����� ���� �� �ְ� ����
ID3D11DeviceContext*	g_pImmediateContext = NULL;				    // GPU�� ���� ����ϸ� �׸��� ����� �����ϴ� ���ؽ�Ʈ
IDXGISwapChain*			g_pSwapChain = NULL;					    // ����ü�� : ȭ�鿡 �������� �̹����� �����ִ� ����. �������� �� ���ۿ��� ����Ʈ ���۷� ��ȯ�ϴµ� ���
ID3D11RenderTargetView* g_pRenderTargetView = NULL;				    // ȭ�鿡 �׸� �� �����͸� ����� ����

HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);		// ������â�� �ʱ�ȭ�ϰ� �����ϴ� �Լ�
HRESULT InitDevice();										// Direct3D ��ġ�� ����ü���� �ʱ�ȭ�ϴ� �Լ�
void CleanupDevice();										// ������ Direct3D ��ü�� �����Ͽ� �޸𸮸� û���ϴ� �Լ�
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);		// â ���ν���, ������ �޽����� ó���ϴ� �Լ�. �����찡 �׷����ų� ����ɶ� ȣ��
void Render();												// �������� �׸��� �Լ�

// ���α׷� ������
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // ������ â ����, ���н� ����
    if (FAILED(InitWindow(hInstance, nCmdShow)))
        return 0;

    // Direct3D ��ġ �ʱ�ȭ, ���н� ���ҽ� ���� �� ����
    if (FAILED(InitDevice()))
    {
        CleanupDevice();
        return 0;
    }

    // ���� �޽��� ����
    // WM_QUIT �޽����� ���ö����� ���α׷� ����.
    // ����ڰ� �ٸ� �޽����� �Է����� ������ Render �Լ��� ȭ���� �׸�
    MSG msg = { 0 };
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Render();
        }
    }

    // ���α׷� ����� ���ҽ� ����
    CleanupDevice();

    return (int)msg.wParam;
}

// ������ �ʱ�ȭ
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    // ������ Ŭ���� ���. â�� ��Ÿ�ϰ� ���ν����� ����
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_TUTORIAL1);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"TutorialWindowClass";
    wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1);
    if (!RegisterClassEx(&wcex))
        return E_FAIL;

    // ������ â ����
    g_hInst = hInstance;
    RECT rc = { 0, 0, 640, 480 };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    g_hWnd = CreateWindow(L"TutorialWindowClass", L"Direct3D 11 Tutorial 1: Direct3D 11 Basics", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
        NULL);
    if (!g_hWnd)
        return E_FAIL;

    // â�� ȭ�鿡 ǥ��
    ShowWindow(g_hWnd, nCmdShow);

    return S_OK;
}

// �޽��� ó��
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    // â�� �׷����ų� â�� ������ �޽��� ó��
    switch (message)
    {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

// Direct3D ��ġ �ʱ�ȭ
HRESULT InitDevice()
{
    // �������� �ʱ�ȭ
    HRESULT hr = S_OK;                  // �Լ��� ��������
    RECT rc;
    GetClientRect(g_hWnd, &rc);         // �������� Ŭ���̾�Ʈ ���� ũ�� ��������. Direct3D ��ġ�� �������� â�� ũ�⸦ ��´�.
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    // Direct3D ��ġ ���� �÷��� ����
    // ����� ��忡�� ���α׷��� �������ϸ� D3D11_CREATE_DEVICE_DEBUG �÷��װ� �����Ǿ� ����� ������ ����. �׷��������� 0���� ����
    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // ����̹� Ÿ�� ����
    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,       // ���� GPU �ϵ��� ����ϴ� ����̹�
        D3D_DRIVER_TYPE_WARP,           // �ϵ���� ������ �Ұ����� ��, ����Ʈ���� ������ ����ϴ� ����̹�
        D3D_DRIVER_TYPE_REFERENCE,      // ���� �ϵ���� ���� ����Ʈ����� ��� ó��. �ַ� �׽�Ʈ������ ���
    };
    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    // Direct3D ��� ���� ���� : Direct3D���� �����ϴ� ��� ������ �����ϴ� �迭
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    // ���� ü�� ���� ����ü �ʱ�ȭ
    DXGI_SWAP_CHAIN_DESC sd;                            // ���� ü�ο� ���� ������ ��� �ִ� ����ü
    ZeroMemory(&sd, sizeof(sd));                        // �޸� �ʱ�ȭ

    sd.BufferCount = 1;                                 // �� ���� ���� 1���� ����
                                                        // �� ���۴� ȭ�鿡 ��µ��� �ʴ� �ĸ� ���۷�, �������� �׸� �� �̸� ���� ���۷� ��ü

    sd.BufferDesc.Width = width;                        // ���� ü���� �� ������ �ʺ� ���� â�� �ʺ�� ����
    sd.BufferDesc.Height = height;                      // ���� ü���� �� ������ ���̸� ���� â�� ���̷� ����

    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // �� ���� �ȼ� ���� ����
                                                        // �� ������ 32��Ʈ ���� ��������, 8��Ʈ�� RGB�� ���� ä�� ��� R8G8B8A8 : red green blue alpha

    sd.BufferDesc.RefreshRate.Numerator = 60;           // ȭ���� ��� �󵵸� 60Hz�� ����, Numerator�� ��� ���� ����
    sd.BufferDesc.RefreshRate.Denominator = 1;          // Denominator�� ��� ���� �и�. �� 60/1�̹Ƿ� 60Hz

    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;   // �� ������ ��� �뵵�� ���� Ÿ�� ������� ����
                                                        // �� ���ۿ� Direct3D�� �׸��� ��� ������ ����� ����

    sd.OutputWindow = g_hWnd;                           // ������ ����� ����� ������ �ڵ��� g_hWnd�� ����
                                                        // �� �ڵ��� ����Ű�� â�� ������ ����� ǥ�É�

    sd.SampleDesc.Count = 1;                            // ��Ƽ���ø��� ��Ȱ��ȭ (���ø� Ƚ���� 1�� ����)
    sd.SampleDesc.Quality = 0;                          // ���ø� ǰ���� �⺻������ ���� (ǰ�� 0)

    sd.Windowed = TRUE;                                 // â ���� ���� (��ü ȭ���� �ƴ� ������ ���� ����)

    // Direct3D ��ġ �� ���� ü�� ���� : ����̹� Ÿ�� �迭�� �ݺ��ϸ� D3D11CreateDeviceAndSwapChain()�Լ��� Direct3D ��ġ �� ���� ü���� ����
    // D3D11CreateDeviceAndSwapChain(�⺻ �����(�׷���ī��), ���� �õ����� ����̹� Ÿ��, ����Ʈ���� ����̹�, ����̽� ���� �÷���, �����ϴ� ��� ���� �迭, 
    // ��� ���� �迭�� ũ��, Direct3D sdk ����, ���� ü�� ����ü ������, ���� ü���� �ּҸ� ���� ������, Direct3D ��ġ, ������ ��� ����, ��ġ ���ؽ�Ʈ)
    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain(NULL, g_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
        if (SUCCEEDED(hr))
            break;
    }
    if (FAILED(hr))
        return hr;

    // �� ���ۿ��� ���� Ÿ�� �� ���� : ���� ü�ο��� �� ���۸� ������ ���� Ÿ�� �並 ����
    ID3D11Texture2D* pBackBuffer = NULL;
    hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);      // ���� ü���� �� ���۸� �������� �Լ�. 0��° ���� ��������
    if (FAILED(hr))
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);     // �� ���۸� ������� ���� Ÿ�� �並 �����.
                                                                                            // �� ��� Direct3D�� �׸��� ����� ����� Ÿ���� �ȴ�.
    pBackBuffer->Release();                                                                 // ������ �� �� ���۴� ����
    if (FAILED(hr))
        return hr;

    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);                 // ��� ���� �ܰ迡�� ����� ���� Ÿ���� ����

    // ����Ʈ ���� : ȭ�鿡 �׸� ������ �����ϴ� �ܰ�
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports(1, &vp);        // ������ ����Ʈ�� ��ġ ������Ʈ�� ����

    // �Լ� ���� : ��� �ʱ�ȭ�� ���������� �Ϸ�Ǹ� S_OK ��ȯ
    return S_OK;
}

// ������
void Render()
{
    // ȭ���� �Ķ�������
    float ClearColor[4] = { 0.0f, 0.0f, 1.0f, 0.0f }; // red, green, blue, alpha
    g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);

    // ȭ�鿡 �׷��� �������� ǥ��
    g_pSwapChain->Present(0, 0);
}

// ����� Direct3D �ڿ����� ��� �����Ͽ� �޸� ����
void CleanupDevice()
{
    if (g_pImmediateContext) g_pImmediateContext->ClearState();

    // Release() �Լ��� ����ؼ� ��� Direct3D ��ü�� ����
    if (g_pRenderTargetView) g_pRenderTargetView->Release();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_pImmediateContext) g_pImmediateContext->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();
}