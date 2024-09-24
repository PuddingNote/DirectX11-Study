#include <windows.h>	// 윈도우 API에 필요한 함수 및 타입을 정의
#include <d3d11.h>		// Direct3D 11에 관련된 모든 기능을 제공
//#include <d3dx11.h>   // Direct3D 도우미 라이브러리로, 텍스처 로드와 같은 작업을 간단하게 해줌 (이 라이브러리는 더 이상 공식적으로 지원되지 않지만 예전 코드에서 자주 사용)
#include "resource.h"

HINSTANCE				g_hInst = NULL;							    // 현재 애플리케이션의 인스턴스 핸들, 윈도우 창을 생성하거나 다른 WIn32 API호출에서 사용
HWND					g_hWnd = NULL;							    // 생성된 윈도우 창의 핸들
D3D_DRIVER_TYPE			g_driverType = D3D_DRIVER_TYPE_NULL;	    // Direct3D의 드라이버 타입. 예) 하드웨어 드라이버, 소프트웨어 드라이버
D3D_FEATURE_LEVEL		g_featureLevel = D3D_FEATURE_LEVEL_11_0;    // Direct3D에서 지원하는 기능 수준
ID3D11Device*			g_pd3dDevice = NULL;					    // Direct3D 11 장치를 나타내는 인터페이스. GPU에 직접 명령을 내릴 수 있게 해줌
ID3D11DeviceContext*	g_pImmediateContext = NULL;				    // GPU와 직접 통신하며 그리기 명령을 관리하는 컨텍스트
IDXGISwapChain*			g_pSwapChain = NULL;					    // 스왑체인 : 화면에 렌더링된 이미지를 보여주는 개념. 프레임을 백 버퍼에서 프론트 버퍼로 전환하는데 사용
ID3D11RenderTargetView* g_pRenderTargetView = NULL;				    // 화면에 그릴 때 데이터를 출력할 공간

HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);		// 윈도우창을 초기화하고 생성하는 함수
HRESULT InitDevice();										// Direct3D 장치와 스왑체인을 초기화하는 함수
void CleanupDevice();										// 생성한 Direct3D 객체를 해제하여 메모리를 청소하는 함수
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);		// 창 프로시저, 윈도우 메시지를 처리하는 함수. 윈도우가 그려지거나 종료될때 호출
void Render();												// 프레임을 그리는 함수

// 프로그램 진입점
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // 윈도우 창 생성, 실패시 종료
    if (FAILED(InitWindow(hInstance, nCmdShow)))
        return 0;

    // Direct3D 장치 초기화, 실패시 리소스 정리 후 종료
    if (FAILED(InitDevice()))
    {
        CleanupDevice();
        return 0;
    }

    // 메인 메시지 루프
    // WM_QUIT 메시지가 들어올때까지 프로그램 실행.
    // 사용자가 다른 메시지를 입력하지 않으면 Render 함수로 화면을 그림
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

    // 프로그램 종료시 리소스 정리
    CleanupDevice();

    return (int)msg.wParam;
}

// 윈도우 초기화
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    // 윈도우 클래스 등록. 창의 스타일과 프로시저를 정의
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

    // 윈도우 창 생성
    g_hInst = hInstance;
    RECT rc = { 0, 0, 640, 480 };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    g_hWnd = CreateWindow(L"TutorialWindowClass", L"Direct3D 11 Tutorial 1: Direct3D 11 Basics", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
        NULL);
    if (!g_hWnd)
        return E_FAIL;

    // 창을 화면에 표시
    ShowWindow(g_hWnd, nCmdShow);

    return S_OK;
}

// 메시지 처리
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    // 창이 그려지거나 창이 닫힐때 메시지 처리
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

// Direct3D 장치 초기화
HRESULT InitDevice()
{
    // 기초적인 초기화
    HRESULT hr = S_OK;                  // 함수의 성공여부
    RECT rc;
    GetClientRect(g_hWnd, &rc);         // 윈도우의 클라이언트 영역 크기 가져오기. Direct3D 장치가 렌더링할 창의 크기를 얻는다.
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    // Direct3D 장치 생성 플래그 설정
    // 디버그 모드에서 프로그램을 컴파일하면 D3D11_CREATE_DEVICE_DEBUG 플래그가 설정되어 디버그 정보를 제공. 그렇지않으면 0으로 설정
    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // 드라이버 타입 설정
    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,       // 실제 GPU 하드웨어를 사용하는 드라이버
        D3D_DRIVER_TYPE_WARP,           // 하드웨어 가속이 불가능할 때, 소프트웨어 가속을 사용하는 드라이버
        D3D_DRIVER_TYPE_REFERENCE,      // 실제 하드웨어 없이 소프트웨어로 모든 처리. 주로 테스트용으로 사용
    };
    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    // Direct3D 기능 수준 설정 : Direct3D에서 지원하는 기능 수준을 지정하는 배열
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    // 스왑 체인 설명 구조체 초기화
    DXGI_SWAP_CHAIN_DESC sd;                            // 스왑 체인에 대한 설정을 담고 있는 구조체
    ZeroMemory(&sd, sizeof(sd));                        // 메모리 초기화

    sd.BufferCount = 1;                                 // 백 버퍼 개수 1개로 설정
                                                        // 백 버퍼는 화면에 출력되지 않는 후면 버퍼로, 프레임을 그린 후 이를 전면 버퍼로 교체

    sd.BufferDesc.Width = width;                        // 스왑 체인의 백 버퍼의 너비를 현재 창의 너비로 설정
    sd.BufferDesc.Height = height;                      // 스왑 체인의 백 버퍼의 높이를 현재 창의 높이로 설정

    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // 백 버퍼 픽셀 형식 지정
                                                        // 이 형식은 32비트 색상 형식으로, 8비트씩 RGB와 알파 채널 사용 R8G8B8A8 : red green blue alpha

    sd.BufferDesc.RefreshRate.Numerator = 60;           // 화면의 재생 빈도를 60Hz로 설정, Numerator는 재생 빈도의 분자
    sd.BufferDesc.RefreshRate.Denominator = 1;          // Denominator는 재생 빈도의 분모. 즉 60/1이므로 60Hz

    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;   // 백 버퍼의 사용 용도를 렌더 타겟 출력으로 설정
                                                        // 이 버퍼에 Direct3D가 그리는 모든 렌더링 결과를 저장

    sd.OutputWindow = g_hWnd;                           // 렌더링 결과를 출력할 윈도우 핸들을 g_hWnd로 설정
                                                        // 이 핸들이 가리키는 창에 렌더링 결과가 표시됌

    sd.SampleDesc.Count = 1;                            // 멀티샘플링을 비활성화 (샘플링 횟수를 1로 설정)
    sd.SampleDesc.Quality = 0;                          // 샘플링 품질을 기본값으로 설정 (품질 0)

    sd.Windowed = TRUE;                                 // 창 모드로 설정 (전체 화면이 아닌 윈도우 모드로 실행)

    // Direct3D 장치 및 스왑 체인 생성 : 드라이버 타입 배열을 반복하며 D3D11CreateDeviceAndSwapChain()함수로 Direct3D 장치 및 스왑 체인을 생성
    // D3D11CreateDeviceAndSwapChain(기본 어댑터(그래픽카드), 현재 시도중인 드라이버 타입, 소프트웨어 드라이버, 디바이스 생성 플래그, 지원하는 기능 수준 배열, 
    // 기능 수준 배열의 크기, Direct3D sdk 버전, 스왑 체인 구조체 포인터, 스왑 체인의 주소를 담을 포인터, Direct3D 장치, 생성된 기능 수준, 장치 컨텍스트)
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

    // 백 버퍼에서 렌더 타겟 뷰 생성 : 스왑 체인에서 백 버퍼를 가져와 렌더 타겟 뷰를 생성
    ID3D11Texture2D* pBackBuffer = NULL;
    hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);      // 스왑 체인의 백 버퍼를 가져오는 함수. 0번째 버퍼 가져오기
    if (FAILED(hr))
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);     // 백 버퍼를 기반으로 렌더 타겟 뷰를 만든다.
                                                                                            // 이 뷰는 Direct3D가 그리는 결과를 출력할 타겟이 된다.
    pBackBuffer->Release();                                                                 // 생성한 후 백 버퍼는 해제
    if (FAILED(hr))
        return hr;

    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);                 // 출력 병합 단계에서 사용할 렌더 타겟을 설정

    // 뷰포트 설정 : 화면에 그릴 영역을 설정하는 단계
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports(1, &vp);        // 설정한 뷰포트를 장치 컨덱스트에 적용

    // 함수 종료 : 모든 초기화가 성공적으로 완료되면 S_OK 반환
    return S_OK;
}

// 렌더링
void Render()
{
    // 화면을 파란색으로
    float ClearColor[4] = { 0.0f, 0.0f, 1.0f, 0.0f }; // red, green, blue, alpha
    g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);

    // 화면에 그려진 프레임을 표시
    g_pSwapChain->Present(0, 0);
}

// 사용한 Direct3D 자원들을 모두 해제하여 메모리 정리
void CleanupDevice()
{
    if (g_pImmediateContext) g_pImmediateContext->ClearState();

    // Release() 함수를 사용해서 모든 Direct3D 객체를 해제
    if (g_pRenderTargetView) g_pRenderTargetView->Release();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_pImmediateContext) g_pImmediateContext->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();
}