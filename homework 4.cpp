//--------------------------------------------------------------------------------------
// File: lecture 8.cpp
//
// This application demonstrates texturing
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "groundwork.h"
#include "explosion.h"
#include "Font.h"
#include "sound.h"
/*
TEAM MEMBERS: 
	Ediberto Cruz: EC
	Salvador Hernandez: SH
	Maria Loza: ML
	Ana Peña: AP

Features Completed:
*****11/3/16*************************************
	Maze Design - Ana created the bitmap from scratch
	Skybox Implementation (Sphere) -Ediberto added the sky sphere
	Shooting Mechanic - Salvador upgraded the shooting to have multiple bullets at once
		Fire Multiple Shots
	Collision Detection with walls - Maria upgraded the walls. The camera no longer passes through the walls.
	Addition of Heads Up Display(HUD) -Maria added the HUD and the ability to change the color.
		HUD displays current color
		Crosshairs in center of screen - Salvador added the crosshair to the frame/HUD texture.
	New wall textures -Ediberto looked up and added the new textures.
	Addition of Enemies
*************************************************

2nd Milestone Completed:
-----11/17/16------------------------------------
	Enemy now moves from 3 different waypoints and doesn't run to into any walls. -Maria and Ana
	Enemy now changes color after a certain amount of time -SH
	Added Explosions when enemy is hit -SH & -EC
	Added sounds when firing bullet -SH & -EC
*/


//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE                           g_hInst = NULL;
HWND                                g_hWnd = NULL;
D3D_DRIVER_TYPE                     g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL                   g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*                       g_pd3dDevice = NULL;
ID3D11DeviceContext*                g_pImmediateContext = NULL;
IDXGISwapChain*                     g_pSwapChain = NULL;
ID3D11RenderTargetView*             g_pRenderTargetView = NULL;
ID3D11Texture2D*                    g_pDepthStencil = NULL;
ID3D11DepthStencilView*             g_pDepthStencilView = NULL;
ID3D11VertexShader*                 g_pVertexShader = NULL;
ID3D11PixelShader*                  g_pPixelShader = NULL;
ID3D11PixelShader*                  g_pPixelShader_frame = NULL; //pixel shader for the frame -ML
ID3D11PixelShader*                  g_pPixelShader_enemies = NULL; //pixel shader for the enemies -SH
ID3D11PixelShader*                  g_pPixelShader_bullets = NULL; //pixel shader for the bullets -SH

ID3D11InputLayout*                  g_pVertexLayout = NULL;
ID3D11Buffer*                       g_pVertexBuffer = NULL;
ID3D11Buffer*                       g_pVertexBuffer_sky = NULL;
//states for turning off and on the depth buffer
ID3D11BlendState*					g_BlendState;//transperancy

ID3D11DepthStencilState			*ds_on, *ds_off;
int									skyCount; //-EC

ID3D11Buffer*                       g_pCBuffer = NULL;

ID3D11ShaderResourceView*           g_pTextureRV = NULL;//to create a new texture 
ID3D11ShaderResourceView*           g_pTextureRV1 = NULL;//to create a new texture 
ID3D11ShaderResourceView*           g_pTextureRV_frame = NULL; //border frame -ML

ID3D11ShaderResourceView*           g_pTextureRV_crosshairs = NULL; //crosshairs -SH

ID3D11ShaderResourceView*           g_pTexture = NULL;


ID3D11SamplerState*                 g_pSamplerLinear = NULL;
XMMATRIX                            g_World;
XMMATRIX                            g_View;
XMMATRIX                            g_Projection;
XMFLOAT4                            g_vMeshColor(0.7f, 0.7f, 0.7f, 1.0f);

camera								cam;
level								level1;
vector<billboard*>					enemies;
vector<bullet*>					    bullets;

//-----enemy movement and their wall collison -ML && AP -----//
float speeding;
float elapsed2;
float distance1;
XMFLOAT3 direction;
XMFLOAT3 startA;
XMFLOAT3 endA;


int nextA;
bool backwardsA = false;
int index;
float idleTime;
//-----------------------------------------------------------

//needed for color change -ML
bool static currColorCheck = false;
XMFLOAT4 frameColor(0, 0, 1, 0);

//needed for bullet color change -SH
bool static spacebar = false;
XMFLOAT4 bulletColor(0, 0, 1, 0);

//needed for enemy color change -SH
XMFLOAT4 enemyColor(0, 0, 1, 0);

//FOR EXPLOSION -SH & -EC
/////////////////////////////
explosion_handler  explosionhandler;

//FOR Font -SH & -EC
/////////////////////////////
Font font;

//-----ememy movement and collison -ML & AP -----------------


enum enemyName {
	A, B, C, D, E
};

struct waypoint {

	int ID;
	XMFLOAT3 position; //where its going

};

vector<vector<waypoint*>> graph; //2D arrayish column, row

waypoint* createWaypoint(XMFLOAT3 in)
{
	waypoint* result = new waypoint();
	result->ID = graph.size();
	result->position = in;
	graph.push_back(vector<waypoint*>());
	return result;
}

XMFLOAT3 b(-2, 0, 8); //A2 1
XMFLOAT3 c(-12, 0, 8); //A3 2

void initGraph() {
	waypoint* w_a = createWaypoint(XMFLOAT3(0, 0, 5)); //A1 0
	waypoint* w_b = createWaypoint(b);
	waypoint* w_c = createWaypoint(c);

	graph[w_a->ID].push_back(w_a);
	graph[w_a->ID].push_back(w_b);
	graph[w_a->ID].push_back(w_c);
	
}

void createEnemies() {
	for (int i = 0; i < 5; i++)
	{
		enemies.push_back(new billboard());
	}
	enemies[0]->position = XMFLOAT3(0, 0, 5); //front A
	enemies[0]->activation = ACTIVE;
	enemies[1]->position = XMFLOAT3(-10, 0, 32); //back B
	enemies[1]->activation = ACTIVE;
	enemies[2]->position = XMFLOAT3(-6, 0, 18); //middle left C
	enemies[2]->activation = ACTIVE;
	enemies[3]->position = XMFLOAT3(3, 0, 14); //middle right D
	enemies[3]->activation = ACTIVE;
	enemies[4]->position = XMFLOAT3(16, 0, 12); //far left E
	enemies[4]->activation = ACTIVE;
	
}

bool active(enemyName enName) {
	if (enName == A) {
		if (enemies[0]->activation == ACTIVE)
			return true;
	}
	else {
		return false;
	}
}

XMFLOAT3 getNextWaypoint(enemyName enName) {

	XMFLOAT3 wayPoint;
	if (enName == A) {
		wayPoint = graph[0][nextA]->position;
		nextA++;
		if (nextA > 2) {
			nextA = 2;
			backwardsA = true;
		}
	}
	return wayPoint;
}

XMFLOAT3 getBeforeWaypoint(enemyName enName) {

	XMFLOAT3 wayPoint;
	if (enName == A) {
		nextA--;
		wayPoint = graph[0][nextA]->position;
		if (nextA < 1) {
			nextA = 1;
			backwardsA = false;
		}
	}
	return wayPoint;
}

void preWalking(enemyName enName) {
	if (enName == A) {
		if (!backwardsA) {
			startA = getNextWaypoint(A);
			endA = getNextWaypoint(A);
		}
	}
}

void startWalking(enemyName enName) {
	bool tempBack;
	XMFLOAT3 start, end1;

	if (enName == A) {
		tempBack = backwardsA;
		index = 0;
		start = startA;
		end1 = endA;
	}
	speeding = 2;
	elapsed2 = 0.01f;


	distance1 = sqrt(pow(end1.x - start.x, 2) + pow(end1.y - start.y, 2) + pow(end1.z - start.z, 2));
	direction = XMFLOAT3((end1.x - start.x) / distance1, (end1.y - start.y) / distance1, (end1.z - start.z) / distance1);

	enemies[index]->position.x += direction.x * speeding * elapsed2;
	enemies[index]->position.y += direction.y * speeding * elapsed2;
	enemies[index]->position.z += direction.z * speeding * elapsed2;

	float currDistance = sqrt(pow(enemies[index]->position.x - start.x, 2) + pow(enemies[index]->position.y - start.y, 2) + pow(enemies[index]->position.z - start.z, 2));

	if (currDistance >= distance1) {

		if (idleTime >= 0.0 && idleTime <= 3.0) {
			enemies[index]->position.x = end1.x;
			enemies[index]->position.y = end1.y;
			enemies[index]->position.z = end1.z;
			idleTime += 0.01;
		}
		else {
			idleTime = 0.0;
			if (!tempBack) {
				if (enName == A) {
					startA = end1;
					endA = getNextWaypoint(enName);
				}
			}
			else {
				if (enName == A) {
					startA = end1;
					endA = getBeforeWaypoint(enName);
				}
			}
		}

	}
}
//----------------------------------------------------------



//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void Render();


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (FAILED(InitWindow(hInstance, nCmdShow)))
		return 0;

	if (FAILED(InitDevice()))
	{
		CleanupDevice();
		return 0;
	}

	//-----for enemy movement and collision -ML & AP----------
	initGraph();
	createEnemies();
	if (active(A)) {
		preWalking(A); //starts walking if enemy is active
	}
	
	//--------------------------------------------------------


	// Main message loop
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

	CleanupDevice();

	return (int)msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	// Register class
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

	// Create window
	g_hInst = hInstance;
	RECT rc = { 0, 0, 1366, 768 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	g_hWnd = CreateWindow(L"TutorialWindowClass", L"Direct3D 11 Tutorial 7", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
		NULL);
	if (!g_hWnd)
		return E_FAIL;

	ShowWindow(g_hWnd, nCmdShow);

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DX11
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

	ID3DBlob* pErrorBlob;
	hr = D3DX11CompileFromFile(szFileName, NULL, NULL, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL);
	if (FAILED(hr))
	{
		if (pErrorBlob != NULL)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		if (pErrorBlob) pErrorBlob->Release();
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

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

	// Create a render target view
	ID3D11Texture2D* pBackBuffer = NULL;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr))
		return hr;

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
		return hr;

	// Create depth stencil texture
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_R32_TYPELESS;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_pd3dDevice->CreateTexture2D(&descDepth, NULL, &g_pDepthStencil);
	if (FAILED(hr))
		return hr;


	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = DXGI_FORMAT_D32_FLOAT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
	if (FAILED(hr))
		return hr;



	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);

	// Compile the vertex shader
	ID3DBlob* pVSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "VS", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the vertex shader
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &g_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Set the input layout
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

	// Compile the pixel shader
	ID3DBlob* pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "PS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	//================================================== compile PS_frame shader -ML ===============================================================//
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "PS_frame", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader named PS_frame
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader_frame);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;
	//================================================================================================================================================//

	//================================================== compile PS_enemies shader -SH ===============================================================//
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "PS_enemies", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader named PS_frame
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader_enemies);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;
	//================================================================================================================================================//

	//================================================== compile PS_bullet shader -SH ===============================================================//
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "PS_bullets", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader named PS_frame
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader_bullets);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;
	//================================================================================================================================================//

	//create skybox vertex buffer
	SimpleVertex vertices_skybox[] =
	{
		//top
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.25f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(0.5f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.5f, 0.33f) },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.25f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.5f, 0.33f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.25f, 0.33f) },

		//bottom
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(0.5f, 0.66f) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.25f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(0.5f, 0.66f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(0.5f, 0.66f) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.25f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.25f, 0.66f) },

		//left
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.25f, 0.33f) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.25f, 0.66f) },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.25f, 0.66f) },
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.25f, 0.33f) },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.25f, 0.66f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.25f, 0.33f) },



		//right
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
		//back
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 1.0f) },
		//front
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
	};
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 36;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices_skybox;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer_sky);
	if (FAILED(hr))
		return hr;
	// Create vertex buffer
	SimpleVertex vertices[] =
	{
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },

		{ XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) }

	};


	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 12;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
	if (FAILED(hr))
		return hr;

	// Set vertex buffer
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);


	// Set primitive topology
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Create the constant buffers
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, NULL, &g_pCBuffer);
	if (FAILED(hr))
		return hr;


	// Load the Texture
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"skb3.jpg", NULL, NULL, &g_pTextureRV, NULL);
	if (FAILED(hr))
		return hr;

	// Load the Texture
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"bullet.png", NULL, NULL, &g_pTextureRV1, NULL);
	if (FAILED(hr))
		return hr;

	// Load the Texture
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"vex.png", NULL, NULL, &g_pTexture, NULL);
	if (FAILED(hr))
		return hr;

	// Load the frame Texture
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"frame_crosshairs3.png", NULL, NULL, &g_pTextureRV_frame, NULL);
	if (FAILED(hr))
		return hr;

	// Create the sample state
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_pSamplerLinear);
	if (FAILED(hr))
		return hr;

	// Initialize the world matrices
	g_World = XMMatrixIdentity();

	// Initialize the view matrix

	//NEW
	XMVECTOR Eye = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);//camera position
	XMVECTOR At = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);//look at
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);// normal vector on at vector (always up)
	g_View = XMMatrixLookAtLH(Eye, At, Up);

	// Initialize the projection matrix
	g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, width / (FLOAT)height, 0.01f, 1000.0f);

	ConstantBuffer constantbuffer;
	constantbuffer.View = XMMatrixTranspose(g_View);
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
	constantbuffer.World = XMMatrixTranspose(XMMatrixIdentity());
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);


	//create the depth stencil states for turning the depth buffer on and of:
	D3D11_DEPTH_STENCIL_DESC		DS_ON, DS_OFF;
	DS_ON.DepthEnable = true;
	DS_ON.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DS_ON.DepthFunc = D3D11_COMPARISON_LESS;
	// Stencil test parameters
	DS_ON.StencilEnable = true;
	DS_ON.StencilReadMask = 0xFF;
	DS_ON.StencilWriteMask = 0xFF;
	// Stencil operations if pixel is front-facing
	DS_ON.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	DS_ON.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	// Stencil operations if pixel is back-facing
	DS_ON.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	DS_ON.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	// Create depth stencil state
	DS_OFF = DS_ON;
	DS_OFF.DepthEnable = false;
	g_pd3dDevice->CreateDepthStencilState(&DS_ON, &ds_on);
	g_pd3dDevice->CreateDepthStencilState(&DS_OFF, &ds_off);

	//level1.init("Bitmap-2.bmp"); //-AP
	level1.init("noRoof.bmp"); //-AP
	//level1.init("level.bmp");
	level1.init_texture(g_pd3dDevice, L"w1.jpg");
	level1.init_texture(g_pd3dDevice, L"w1.jpg");
	level1.init_texture(g_pd3dDevice, L"floor.jpg");
	level1.init_texture(g_pd3dDevice, L"ceiling.jpg");


	D3D11_BLEND_DESC blendStateDesc;
	ZeroMemory(&blendStateDesc, sizeof(D3D11_BLEND_DESC));
	blendStateDesc.AlphaToCoverageEnable = FALSE;
	blendStateDesc.IndependentBlendEnable = FALSE;
	blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;
	g_pd3dDevice->CreateBlendState(&blendStateDesc, &g_BlendState);


	float blendFactor[] = { 0, 0, 0, 0 };
	UINT sampleMask = 0xffffffff;
	g_pImmediateContext->OMSetBlendState(g_BlendState, blendFactor, sampleMask);

	//added for sky sphere -EC
	LoadCatmullClark(L"ccsphere.cmp", g_pd3dDevice, &g_pVertexBuffer_sky, &skyCount);


	//ADDED FOR FONT -SH
	//////////////////////////
	font.init(g_pd3dDevice, g_pImmediateContext, font.defaultFontMapDesc);
	//FOR EXPLOSION
	/////////////////////////////////////////
	explosionhandler.init(g_pd3dDevice, g_pImmediateContext);

	//initialize explosion
	explosionhandler.init_types(L"exp1.dds", 8, 8, 1000000);		                  //<-1. argument: filename of the animated image
		//																			   2. argument: count of subparts of the image in x
		//																			   3. argument: count of subparts of the image in y
		//																			   4. argument: lifespan in microsecond
	//////////////////////////////////////////////
	return S_OK;
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
	if (g_pImmediateContext) g_pImmediateContext->ClearState();

	if (g_pSamplerLinear) g_pSamplerLinear->Release();
	if (g_pTextureRV) g_pTextureRV->Release();
	if (g_pTextureRV_frame) g_pTextureRV_frame->Release();
	if (g_pTextureRV_crosshairs) g_pTextureRV_crosshairs->Release(); //-ML

	if (g_pCBuffer) g_pCBuffer->Release();
	if (g_pVertexBuffer) g_pVertexBuffer->Release();
	if (g_pVertexLayout) g_pVertexLayout->Release();
	if (g_pVertexShader) g_pVertexShader->Release();
	if (g_pPixelShader) g_pPixelShader->Release();
	if (g_pDepthStencil) g_pDepthStencil->Release();
	if (g_pDepthStencilView) g_pDepthStencilView->Release();
	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
}
///////////////////////////////////
//		This Function is called every time the Left Mouse Button is down
///////////////////////////////////
bullet *bull = NULL;
int totalBullets = 0;
bool shoot = false;
void OnLBD(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	shoot = true;
	//MULTIPLE BULLETS -SH
	///////////////////////////////////////////////////
	//creates a new bullet, assigns the values and pushed it to the bullets vector
	//apply a new billboard
	bullet *new_bull = new bullet;
	new_bull->pos.x = -cam.position.x;
	new_bull->pos.y = -cam.position.y;
	new_bull->pos.z = -cam.position.z;

	XMMATRIX CR = XMMatrixRotationY(-cam.rotation.y);

	XMFLOAT3 forward = XMFLOAT3(0, 0, 3);
	XMVECTOR f = XMLoadFloat3(&forward);
	f = XMVector3TransformCoord(f, CR);
	XMStoreFloat3(&forward, f);

	new_bull->imp = forward;
	bullets.push_back(new_bull);

	//added for sound effect
	//still needs work, it crashes when the sound is done playing
	start_music(L"shotgunBlast.mp3");
}
///////////////////////////////////
//		This Function is called every time the Right Mouse Button is down
///////////////////////////////////
//ConstantBuffer cb;
void OnRBD(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	//Color Changer -ML
	///////////////////////////////////////
	if (currColorCheck == false) {
		//send to buffer to change color to the next one, red
		currColorCheck = true;
		frameColor = XMFLOAT4(1, 0, 0, 0);
	}
	else if (currColorCheck == true) {
		//send to buffer for og color, blue
		currColorCheck = false;
		frameColor = XMFLOAT4(0, 0, 1, 0);
	}
	//END Color Changer
	////////////////////////////////////////
}
///////////////////////////////////
//		This Function is called every time a character key is pressed
///////////////////////////////////
void OnChar(HWND hwnd, UINT ch, int cRepeat)
{

}
///////////////////////////////////
//		This Function is called every time the Left Mouse Button is up
///////////////////////////////////
void OnLBU(HWND hwnd, int x, int y, UINT keyFlags)
{


}
///////////////////////////////////
//		This Function is called every time the Right Mouse Button is up
///////////////////////////////////
void OnRBU(HWND hwnd, int x, int y, UINT keyFlags)
{


}
///////////////////////////////////
//		This Function is called every time the Mouse Moves
///////////////////////////////////

int callsetcur = 0;
void OnMM(HWND hwnd, int x, int y, UINT keyFlags)
{
	//THIS KEEPS THE MOUSE IN THE MIDDLE
	//-ML & -SH
	/////////////////////////////////////////
	static int holdx = x, holdy = y;
	static int resetcur = 0;
	RECT rc; 			//rectange structure
	GetWindowRect(hwnd, &rc); 	//retrieves the window size
	int border = 20;
	rc.bottom -= border;
	rc.right -= border;
	rc.left += border;
	rc.top += border;
	ClipCursor(&rc);


	if (callsetcur == 1) {
		callsetcur = 0;
		return;
	}

	ShowCursor(false); //hides cursor -ML

					   /*
					   static int lastx = x;
					   int diff = lastx - x;

					   lastx = x;

					   //rotates according to mouse
					   cam.rotation.y += diff * 0.023;
					   */

	if ((keyFlags & MK_LBUTTON) == MK_LBUTTON)
	{
	}

	if ((keyFlags & MK_RBUTTON) == MK_RBUTTON)
	{
	}
	if (resetcur == 1)
	{
		resetcur = 0;
		holdx = x;
		holdy = y;
		return;
	}

	int diffx = holdx - x;
	float angle_y = (float)diffx / 300.0;
	cam.rotation.y += angle_y;

	int midx = (rc.left + rc.right) / 2;
	int midy = (rc.top + rc.bottom) / 2;
	SetCursorPos(midx, midy);
	resetcur = 1;
	//END
	//////////////////////////////////////////////////
}


BOOL OnCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct)
{
	/*
	for (int i = 0; i < 5; i++)
	{
		enemies.push_back(new billboard());
	}
	enemies[0]->position = XMFLOAT3(0, 0, 5);
	enemies[1]->position = XMFLOAT3(-2, 0,8.5 );
	enemies[2]->position = XMFLOAT3(2, 0, 8.5);
	enemies[3]->position = XMFLOAT3(2, 0, 12);
	enemies[4]->position = XMFLOAT3(-3, 0, 12);
	*/

	RECT rc;
	GetWindowRect(hwnd, &rc);
	int midx = (rc.right + rc.left) / 2;
	int midy = (rc.bottom + rc.top) / 2;
	SetCursorPos(midx, midy);

	return TRUE;
}
void OnTimer(HWND hwnd, UINT id)
{

}
//*************************************************************************
void OnKeyUp(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	switch (vk)
	{
	case 16: cam.shift = false; //shift
		break;
	case 65:cam.a = 0;//a
		break;
	case 68: cam.d = 0;//d
		break;
	case 32: //space
		break;
	case 87: cam.w = 0; //w
		break;
	case 83:cam.s = 0; //s
	default:break;

	}

}

void OnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{

	switch (vk)
	{
	default:break;
		
	case 16: cam.shift = true;//shift
		break;
	case 65:cam.a = 1;//a
		break;
	case 68: cam.d = 1;//d
		break;
	case 32: //space

		//CHANGES BULLET COLOR -SH
		////////////////////////////////////
		if(spacebar == false) {
			bulletColor = XMFLOAT4(1, 0, 0, 0);
			spacebar = true;
		}
		else {
			bulletColor = XMFLOAT4(0, 0, 1, 0);
			spacebar = false;
		}
		////////////////////////////////////
		break;
	case 87: cam.w = 1; //w
		break;
	case 83:cam.s = 1; //s
		break;
	case 27:
		PostQuitMessage(0);//escape
		break;
	}
}

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
#include <windowsx.h>
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
		HANDLE_MSG(hWnd, WM_LBUTTONDOWN, OnLBD);
		HANDLE_MSG(hWnd, WM_RBUTTONDOWN, OnRBD); // RBD added -ML
		HANDLE_MSG(hWnd, WM_LBUTTONUP, OnLBU);
		HANDLE_MSG(hWnd, WM_MOUSEMOVE, OnMM);
		HANDLE_MSG(hWnd, WM_CREATE, OnCreate);
		HANDLE_MSG(hWnd, WM_TIMER, OnTimer);
		HANDLE_MSG(hWnd, WM_KEYDOWN, OnKeyDown);
		HANDLE_MSG(hWnd, WM_KEYUP, OnKeyUp);
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



//--------------------------------------------------------------------------------------
// sprites
//--------------------------------------------------------------------------------------
class sprites
{
public:
	XMFLOAT3 position;
	XMFLOAT3 impulse;
	float rotation_x;
	float rotation_y;
	float rotation_z;
	sprites()
	{
		impulse = position = XMFLOAT3(0, 0, 0);
		rotation_x = rotation_y = rotation_z;
	}
	XMMATRIX animation()
	{
		//update position:
		position.x = position.x + impulse.x; //newtons law
		position.y = position.y + impulse.y; //newtons law
		position.z = position.z + impulse.z; //newtons law

		XMMATRIX M;
		//make matrix M:
		XMMATRIX R, Rx, Ry, Rz, T;
		T = XMMatrixTranslation(position.x, position.y, position.z);
		Rx = XMMatrixRotationX(rotation_x);
		Ry = XMMatrixRotationX(rotation_y);
		Rz = XMMatrixRotationX(rotation_z);
		R = Rx*Ry*Rz;
		M = R*T;
		return M;
	}
};
sprites mario;

/*distance_vector = pos_object_a – pos_object_b

float length = √(〖"distance_vector.x" 〗^2 " + " 〖"distance_vector." 𝑦〗^2 )

if(length<minimum_length)*/

//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------
boolean colorSwitch = true;

//added for enemy color change -SH
/////////////////////////////////
int minSecs = 10;
int maxSecs = 15;
void Render()
{
	static StopWatchMicro_ stopwatch;
	long elapsed = stopwatch.elapse_micro();
	//long elapsedMillis = stopwatch.elapse_milli();
	stopwatch.start();//restart
	//timer += elapsed 

	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	// Update our time
	static float t = 0.0f;
	
	//in the future change it so that the timer based on actual seconds
	//changed to represent secs
	t += 0.01;

	//t equals seconds
	//CHANGE ENEMY COLOR -SH
	///////////////////////////
	int range = maxSecs - minSecs + 1;
	int num = rand() % range + minSecs;

	//FIX THE TIMING IN THE FUTURE
	if (t >= num) {
		if(colorSwitch)
			enemyColor = XMFLOAT4(1, 0, 0, 0);
		else
			enemyColor = XMFLOAT4(0, 0, 1, 0);
		
		colorSwitch = !colorSwitch;
		t = 0;
	}
	///////////////////////////

	// Clear the back buffer
	float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; // red, green, blue, alpha
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);

	// Clear the depth buffer to 1.0 (max depth)
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	cam.animation(&level1);//pass the level -ML

	if (active(A)) { //continues walking if enemy is active -ML
		startWalking(A); //enemy movement and collision -ML & AP
	}
	

	XMMATRIX view = cam.get_matrix(&g_View);
	XMMATRIX worldmatrix;

	static billboard billene;

	//bill.position = rocket_position;



	// Update skybox constant buffer
	ConstantBuffer constantbuffer;
	constantbuffer.World = XMMatrixTranspose(XMMatrixScaling(1, 1, 1));
	constantbuffer.View = XMMatrixTranspose(view);
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

	// Render skybox
	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureRV);
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer_sky, &stride, &offset);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
	g_pImmediateContext->Draw(skyCount, 0); //-EC
	//g_pImmediateContext->Draw(36, 0);



	//render all the walls of the level
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	level1.render_level(g_pImmediateContext, g_pVertexBuffer, &view, &g_Projection, g_pCBuffer);


	billene.position.z = 3;
	billene.position.x = 2;

	// Present our back buffer to our front buffer
	XMMATRIX VR = billene.get_matrix(view);


	constantbuffer.World = XMMatrixTranspose(VR);
	constantbuffer.View = XMMatrixTranspose(view);
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);//pushes data to the next lines osea buffer// Render billboard



	//ENEMIES
	//enemies have their own pixel shader
	//////////////////////////////////////////////////



	//to pass in the proper color for the enemy -SH
	constantbuffer.enemyColorChanger = enemyColor;

	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	//changed the pixelshader  to -SH
	g_pImmediateContext->PSSetShader(g_pPixelShader_enemies, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTexture);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

	//DRAW ENEMIES
	for (int i = 0; i < enemies.size(); i++) {
		if (enemies[i]->activation == ACTIVE) { //draws enemies if they are active -ML
			VR = enemies[i]->get_matrix(view);

			//ENEMY COLLISION WITH WALL -SH
			//////////////////////////////////////////////
			if (level1.isWalkable(enemies[i]->position)) {
				//do something when it hits a wall

			}
			//////////////////////////////////////////////


			constantbuffer.World = XMMatrixTranspose(VR);
			g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);
			g_pImmediateContext->Draw(12, 0);
		}
		
	}

	///////////////////////////////////////////

	//FIRE MULTIPLE BULLETS -SH
	//////////////////////////////////////
	for (int ii = 0; ii < bullets.size(); ii++)
	{
		worldmatrix = bullets[ii]->getmatrix(elapsed, view);
		
		//if the player does not like to have to switch bullet colors as well
		//change bulletColor to frameColor
		//bulletColor
		constantbuffer.bulletColorChanger = frameColor; //sends the current bullet color to the Pixel Shader -SH
		g_pImmediateContext->PSSetShader(g_pPixelShader_bullets, NULL, 0); //added pixel shader for bullets

		g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureRV1);
		constantbuffer.World = XMMatrixTranspose(worldmatrix);
		constantbuffer.View = XMMatrixTranspose(view);
		constantbuffer.Projection = XMMatrixTranspose(g_Projection);
		constantbuffer.info = XMFLOAT4(1, 1, 1, 1);
		//constantbuffer.info = XMFLOAT4(smokeray[ii]->transparency, 1, 1, 1);
		g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);


		//ENEMY BULLET COLLISION & PLAYER COLLISION WITH ENEMY
		//SH & AP
		//////////////////////////////////////////////////////////////
		XMFLOAT3 dPBE;

		for (int jj = 0; jj < enemies.size(); jj++) {
			dPBE.x = bullets[ii]->pos.x - enemies[jj]->position.x;
			dPBE.y = bullets[ii]->pos.y - enemies[jj]->position.y;
			dPBE.z = bullets[ii]->pos.z - enemies[jj]->position.z;

			//ok, now calculate the length of the vector :
			float lengthPBE = sqrt(dPBE.x* dPBE.x + dPBE.y* dPBE.y + dPBE.z* dPBE.z);
			
			//Delete the enemy on hit -EC
			//////////////////////////
			if (lengthPBE < 0.3)
			{
				//enemies.pop_back();
				//erase the eneny 
				//enemies.erase(enemies.begin()+ jj);

				//ADDS FIRE TO THE ENEMY
				//-SH & -EC
				//////////////////////////////
				explosionhandler.new_explosion(XMFLOAT3(bullets[ii]->pos.x, bullets[ii]->pos.y, bullets[ii]->pos.z), XMFLOAT3(0, 0, 0), 1, 4.0);//<-1. argument: position
																																				//2. argument: impulse in unit per second
																															   				    //3. argument: type of explosions (how many have you initialized?) starting with 0
																																			    //4. argument: scaling of the explosion
																																			    /////////////////////
				
				//decreases enemy life and checks if they have no life left -SH
				if (--enemies[jj]->life <= 0) {
					enemies[jj]->activation = INACTIVE; //once bullet hits enemy, the enemy is inactive to be drawn -ML
				}

			}

		//END ENEMY BULLET COLLISION
		////////////////////////////////////////////////////////////////
		}

		g_pImmediateContext->Draw(12, 0);
	}
	//END FIRE MULTIPLE BULLETS
	/////////////////////////////////////////////////////////////

	//set before the frame because otherwise it will interfere with the frame itself
	//FOR EXPLOSION
	//////////////////////////////////////////////
	g_pImmediateContext->OMSetDepthStencilState(ds_off, 1);
	explosionhandler.render(&view, &g_Projection, elapsed);
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);
	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);
	//////////////////////////////////////////////

	
	//================================================ border frame for camera -ML =======================================================//
	// Update frame constant buffer
	//World, View, and Projection needs to have the Identity matrix to have the texture on the screen
	constantbuffer.World = XMMatrixTranspose(XMMatrixIdentity());
	constantbuffer.View = XMMatrixTranspose(XMMatrixIdentity());
	constantbuffer.Projection = XMMatrixTranspose(XMMatrixIdentity());
	constantbuffer.colorChanger = frameColor; //sends the current frame color, can be manupilated with RBD -ML
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

	// Render frame
	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader_frame, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureRV_frame); //this texture goes to a different PS shader than the default
																		  //g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer_sky, &stride, &offset);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);


	g_pImmediateContext->Draw(36, 0);
	//=================================================================================================================================//

	//DISPLAYS INFORMATION -EC & -SH
	/////////////////////////////////
	//it slows down the program
	//font << "TEST";

	g_pSwapChain->Present(0, 0);
}
