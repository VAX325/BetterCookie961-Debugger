#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>

#include <portable-file-dialogs/portable-file-dialogs.h>

#include <d3d11.h>
#include <tchar.h>
#include <Windows.h>

#include <iostream>
#include <cstdarg>
#include <vector>
#include <stack>
#include <string>
#include <functional>
#include <memory>
#include <istream>
#include <sstream>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "imgui_utils.h"
#include "BCookie961.h"
#include "utils.h"
#include "resource.h"

using namespace std::string_literals;

constexpr auto g_iInitialWidth = 1366;
constexpr auto g_iInitialHeight = 768;

void Fatal(const char* const msg, ...)
{
	va_list va;
	va_start(va, msg);
	size_t size = std::vsnprintf(nullptr, 0, msg, va) + 1;
	va_end(va);

	std::vector<char> buffer(size);
	va_start(va, msg);
	std::vsnprintf(buffer.data(), size, msg, va);
	va_end(va);

	std::fprintf(stderr, "%s\n", buffer.data());
	::MessageBoxA(NULL, buffer.data(), "ERROR!", MB_OK);

	std::exit(1);
}

struct RenderContext_t
{
	HWND m_hWnd;
	uint32_t m_iWidth;
	uint32_t m_iHeight;

	IDXGISwapChain* m_pSwapChain;

	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pContext;

	ID3D11RenderTargetView* m_pRenderTargetView;

	ImFont* m_pBC961Font;
	ImFont* m_pDefaultFont;
};

class IWindow
{
public:
	IWindow() = default;
	virtual ~IWindow() = default;

	virtual void Render(RenderContext_t& context) = 0;
};
using Windows_t = std::vector<std::unique_ptr<IWindow>>;

class CMainLayoutWindow final : public IWindow
{
private:
	const int m_iCellWidth = 80;
	const int m_iCellHeight = 60;

	bool m_bNeedScroll;
	std::string m_ConsoleBuffer;

	int m_iHistoryPos;
	std::vector<std::string> m_vConsoleHistory;

	COutStreamBuf m_COUTStreamBuf;
	CInStreamBuf m_CINStreamBuf;

	std::atomic_bool m_bExecute;
	std::atomic_bool m_bIntepreterThreadFileRestart;
	std::jthread m_InterpreterThread;

public:
	CMainLayoutWindow()
		: m_bNeedScroll(false), m_iHistoryPos(-1), m_bExecute(true),
		  m_COUTStreamBuf([](void* userData, char c) -> void
						  { static_cast<CMainLayoutWindow*>(userData)->m_bNeedScroll = true; },
						  this)
	{
		RestartInterpreter();
	}

	virtual ~CMainLayoutWindow()
	{
		m_COUTStreamBuf.Close();
		m_CINStreamBuf.Close();

		if (m_bExecute.exchange(false) && m_InterpreterThread.joinable()) m_InterpreterThread.join();
	}

	void Render(RenderContext_t& context) override
	{
		ImGui::SetNextWindowPos({0, 0});
		ImGui::SetNextWindowSize({(float)context.m_iWidth, m_iCellHeight + 40.f});

		ImGui::PushFont(context.m_pDefaultFont);
		const int cellsCount = getArray().size();
		if (ImGui::Begin("Cells", nullptr,
						 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
							 ImGuiWindowFlags_HorizontalScrollbar))
		{
			ImVec2 window_pos = ImGui::GetWindowPos();
			ImVec2 window_size = ImGui::GetWindowSize();
			ImVec2 window_center = ImVec2(window_pos.x + window_size.x * 0.5f, window_pos.y + window_size.y * 0.5f);

			const auto& color = ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered];

			float posX;
			if (cellsCount * m_iCellWidth > ImGui::GetWindowSize().x)
				posX = m_iCellWidth / 2.f;
			else
				posX = ImGui::GetWindowSize().x / 2.f - ((cellsCount - 1) * m_iCellWidth / 2.f);

			for (int i = 0; i < cellsCount; ++i)
			{
				DrawCell(getArray().at(i), {posX, window_center.y}, i == getPointerLocation());
				posX += m_iCellWidth;
			}

			ImGui::End();
		}
		ImGui::PopFont();

		ImGui::SetNextWindowPos({0, m_iCellHeight + 40.f});
		ImGui::SetNextWindowSize({context.m_iWidth - 350.f, context.m_iHeight - (m_iCellHeight + 40.f)});
		if (ImGui::Begin("Debugger", nullptr,
						 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
		{
			ImGui::PushFont(context.m_pBC961Font);

			ImVec2 size = ImGui::GetWindowSize();
			size.x -= ImGui::GetStyle().FramePadding.x * 4;
			size.y -= (ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 8.5f);
			if (ImGui::BeginChild("STDOUT", size, ImGuiChildFlags_Border, ImGuiWindowFlags_HorizontalScrollbar))
			{
				ImGui::TextUnformatted(m_COUTStreamBuf.str().c_str());
				if (m_bNeedScroll)
				{
					ImGui::SetScrollHereY(1.0f);
					m_bNeedScroll = false;
				}

				ImGui::EndChild();
			}

			ImGui::PushItemWidth(size.x);
			if (ImGui::InputTextWithHint(
					"##Console", "Enter text here!", &m_ConsoleBuffer,
					ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AllowTabInput |
						ImGuiInputTextFlags_CallbackHistory,
					[](ImGuiInputTextCallbackData* data)
					{ return static_cast<CMainLayoutWindow*>(data->UserData)->ConsoleCallback(data); },
					this))
			{
				ImGui::SetKeyboardFocusHere(-1); // Do not lose focus

				if (m_ConsoleBuffer.size())
				{
					std::cout << m_ConsoleBuffer + '\n';
					m_bNeedScroll = true;

					m_CINStreamBuf.provide_data(m_ConsoleBuffer + '\n');

					m_iHistoryPos = -1;
					m_vConsoleHistory.emplace_back(m_ConsoleBuffer);
					m_ConsoleBuffer.clear();
				}
			}
			ImGui::PopItemWidth();
			ImGui::PopFont();

			ImGui::End();
		}

		extern std::atomic_bool debuggerWait;
		extern std::atomic_bool debuggerStep;
		extern std::string_view debuggerCurrentCode;
		extern std::string_view::size_type debuggerCurrentCodePos;
		static bool needToScrollCode = false;

		ImGui::SetNextWindowPos({context.m_iWidth - 350.f, m_iCellHeight + 40.f});
		ImGui::SetNextWindowSize({350.f, context.m_iHeight - (m_iCellHeight + 40.f)});
		if (ImGui::Begin("Tools", nullptr,
						 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
		{
			if (ImGui::Button("Restart BC961 Interpreter\n(On deadlock or relaunch)")) 
			{
				// ImGui popup system is very tricky
				ImGui::OpenPopup("Really?"); 
			}

			if (ImGui::Button("Execute file"))
			{
				auto result = pfd::open_file::open_file("Choose source file", ".", {"BC961 Source files", "*.bc961"});
				while (!result.ready())
					;

				if (result.result().size())
					RestartInterpreterAndExecFile(result.result().back());
			}

			// Debugger extensions
			{
				if (ImGui::Button("Stop/Play (F5)") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_F5)))
					debuggerWait = !debuggerWait;

				ImGui::RadioButton("Is execution stoped?", debuggerWait);

				ImGui::SeparatorText("Debugger tools");
				if (!debuggerWait) ImGui::BeginDisabled(true);
				{
					if (ImGui::Button("Execute one step (F10)") ||
						ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_F10)))
					{
						needToScrollCode = true;
						debuggerStep = true;
					}
				}
				if (!debuggerWait) ImGui::EndDisabled();
			}

			if (ImGui::BeginPopupModal("Really?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("This will reset interpreter to initial state. Continue?");

				if (ImGui::Button("Hell yea", ImVec2(120, 40)))
				{
					RestartInterpreter();
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Nah...", ImVec2(120, 40)))
				{
					// Does nothing _/-_^\_
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			if (!debuggerWait) ImGui::BeginDisabled(true);
			{
				// ImGui::Text("Current position in code: %zi", debuggerCurrentCodePos);
				// if (ImGui::BeginListBox("##Breakpoints"))
				//{
				//	// setBreakpoint();
				//	ImGui::EndListBox();
				// }
				// ImGui::IsItemClicked();

				if (ImGui::BeginChild("Code", ImVec2(0, 0), ImGuiChildFlags_Border,
									  ImGuiWindowFlags_HorizontalScrollbar))
				{
					if (debuggerCurrentCode.size())
					{
						ImGui::Text("%s", std::string(debuggerCurrentCode.substr(0, debuggerCurrentCodePos)).c_str());

						ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_ButtonActive], "%s",
										   std::string(debuggerCurrentCode.substr(debuggerCurrentCodePos, 1)).c_str());

						if (needToScrollCode)
						{
							ImGui::SetScrollHereX();
							ImGui::SetScrollHereY();
							needToScrollCode = false;
						}

						if (debuggerCurrentCodePos + 1 < debuggerCurrentCode.size())
						{
							ImGui::Text("%s",
										std::string(debuggerCurrentCode.substr(debuggerCurrentCodePos + 1)).c_str());
						}
					}
					ImGui::EndChild();
				}
			}
			if (!debuggerWait) ImGui::EndDisabled();

			ImGui::End();
		}
	}

private:
	void RestartRoutine() 
	{
		if (m_InterpreterThread.joinable())
		{
			const auto [c, u] = m_COUTStreamBuf.Close();
			m_CINStreamBuf.Close();

			m_bExecute.exchange(false);
			m_InterpreterThread.join();

			m_COUTStreamBuf.Open(c, u);
			m_CINStreamBuf.Open();
		}

		std::cout.rdbuf(&m_COUTStreamBuf);
		std::cin.rdbuf(&m_CINStreamBuf);

		m_bExecute.exchange(true);
	}

	void RestartInterpreter()
	{
		RestartRoutine();

		m_InterpreterThread = std::jthread(
			[this]()
			{
				while (m_bExecute)
				{
					try
					{
						(void)bc961_main_shell(&m_bExecute);
					}
					catch (CExitException&)
					{
					}
					catch (std::exception& e)
					{
						std::cout << "Interpreter ended with this exception: " << e.what() << std::endl;
					}

					std::cout << "Enter anything to restart interpreter" << std::endl;

					std::string s;
					while (s.size() == 0 && m_bExecute)
						std::cin >> s;

					const auto [c, u] = m_COUTStreamBuf.Close();
					m_COUTStreamBuf.Open(c, u);
				}
			});
	}

	void RestartInterpreterAndExecFile(const std::string_view filename)
	{
		RestartRoutine();

		std::jthread awaiter = std::jthread([this]() 
		{ 
			using namespace std::chrono_literals;

			if (m_InterpreterThread.joinable())
					m_InterpreterThread.join();

			if (m_bIntepreterThreadFileRestart)
			{
				RestartInterpreter();
				m_bIntepreterThreadFileRestart = false;
			}
		});
		awaiter.detach();
		
		m_InterpreterThread = std::jthread(
			[this](const std::string filename)
			{
				try
				{
					(void)bc961_main_file(&m_bExecute, filename);
				}
				catch (CExitException&)
				{
				}
				catch (std::exception& e)
				{
					std::cout << "Interpreter ended with this exception: " << e.what() << std::endl;
				}

				std::cout << "Enter anything to restart interpreter" << std::endl;

				std::string s;
				while (s.size() == 0 && m_bExecute)
					std::cin >> s;

				const auto [c, u] = m_COUTStreamBuf.Close();
				m_COUTStreamBuf.Open(c, u);

				if (s.size()) m_bIntepreterThreadFileRestart = true;
			}, std::string(filename));
	}

	int ConsoleCallback(ImGuiInputTextCallbackData* data)
	{
		if (data->EventFlag & ImGuiInputTextFlags_CallbackHistory)
		{
			const int prev_history_pos = m_iHistoryPos;

			if (data->EventKey == ImGuiKey_UpArrow)
			{
				if (m_iHistoryPos == -1)
					m_iHistoryPos = (int)m_vConsoleHistory.size() - 1;
				else if (m_iHistoryPos > 0)
					m_iHistoryPos--;
			}
			else if (data->EventKey == ImGuiKey_DownArrow)
			{
				if (m_iHistoryPos != -1)
					if (++m_iHistoryPos >= (int)m_vConsoleHistory.size()) m_iHistoryPos = -1;
			}

			if (prev_history_pos != m_iHistoryPos)
			{
				const auto& history_str = (m_iHistoryPos >= 0) ? m_vConsoleHistory[m_iHistoryPos] : "";
				data->DeleteChars(0, data->BufTextLen);
				data->InsertChars(0, history_str.c_str());
			}
		}

		return 0;
	}

	void DrawCell(int value, const ImVec2& pos, bool current = false)
	{
		float scroll = ImGui::GetScrollX();
		if (!current)
			ImGuiUtils::DrawRect({pos.x - scroll, pos.y}, {(float)m_iCellWidth, (float)m_iCellHeight},
								 ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
		else
			ImGuiUtils::DrawRectFilled({pos.x - scroll, pos.y}, {(float)m_iCellWidth, (float)m_iCellHeight},
									   ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered],
									   ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
		ImGui::SetCursorPos(pos);
		if (std::isprint((char)value))
			ImGuiUtils::TextCentered("%i\n'%c'", value, (char)value);
		else
			ImGuiUtils::TextCentered("%i", value);
	}
};

static uint32_t g_iResizeWidth = 0;
static uint32_t g_iResizeHeight = 0;
static inline LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return TRUE;

	switch (msg)
	{
		case WM_SIZE:
			if (wParam == SIZE_MINIMIZED) return 0;
			g_iResizeWidth = (UINT)LOWORD(lParam);
			g_iResizeHeight = (UINT)HIWORD(lParam);
			break;

		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
				return FALSE;
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			return FALSE;

		default:
			break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

static inline HWND CreateDebuggerWindow()
{
	const TCHAR CLASS_NAME[] = _TEXT("BetterCookie961DebuggerWindow");
	WNDCLASS wc = {
		.lpfnWndProc = WndProc,
		.hInstance = GetModuleHandle(0),
		.hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_ICON1)),
		.lpszClassName = CLASS_NAME,
	};
	RegisterClass(&wc);

	int nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	int nScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	const std::string sas = "BetterCookie961 Debugger (version "s + DebuggerVersion + ")";
	HWND hwnd = CreateWindowEx(0, CLASS_NAME, std::wstring(sas.begin(), sas.end()).c_str(), WS_OVERLAPPEDWINDOW,
							   nScreenWidth / 2 - g_iInitialWidth / 2, nScreenHeight / 2 - g_iInitialHeight / 2,
							   g_iInitialWidth, g_iInitialHeight, NULL, NULL, GetModuleHandle(0), NULL);

	if (!hwnd) Fatal("Can't create window! Error: %d", GetLastError());

	ShowWindow(hwnd, 1);

	return hwnd;
}

static inline void CreateRenderTarget(RenderContext_t& context)
{
	ID3D11Texture2D* pBackBuffer;
	HRESULT hr = context.m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	if (FAILED(hr)) Fatal("Can't get back buffer!");

	hr = context.m_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &context.m_pRenderTargetView);
	if (FAILED(hr)) Fatal("Can't create render target!");

	pBackBuffer->Release();

	context.m_pContext->OMSetRenderTargets(1, &context.m_pRenderTargetView, nullptr);
}

static inline void CleanupRenderTarget(RenderContext_t& context)
{
	if (context.m_pRenderTargetView)
	{
		context.m_pRenderTargetView->Release();
		context.m_pRenderTargetView = nullptr;
	}
}

static inline void InitDirectX11(RenderContext_t& context)
{
	RECT renderSurfaceRct;
	GetClientRect(context.m_hWnd, &renderSurfaceRct);

	DXGI_SWAP_CHAIN_DESC sd;
	{
		ZeroMemory(&sd, sizeof(sd));
		sd.BufferCount = 1;
		sd.BufferDesc.Width = renderSurfaceRct.right;
		sd.BufferDesc.Height = renderSurfaceRct.bottom;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = context.m_hWnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;
	}

	D3D_FEATURE_LEVEL FeatureLevelsRequested = D3D_FEATURE_LEVEL_11_0;
	UINT numLevelsRequested = 1;
	D3D_FEATURE_LEVEL FeatureLevelsSupported;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, &FeatureLevelsRequested, 1,
											   D3D11_SDK_VERSION, &sd, &context.m_pSwapChain, &context.m_pDevice,
											   &FeatureLevelsSupported, &context.m_pContext);

	D3D11_VIEWPORT vp = {
		.TopLeftX = 0,
		.TopLeftY = 0,
		.Width = (FLOAT)renderSurfaceRct.right,
		.Height = (FLOAT)renderSurfaceRct.bottom,
		.MinDepth = 0.0f,
		.MaxDepth = 1.0f,
	};
	context.m_pContext->RSSetViewports(1, &vp);

	CreateRenderTarget(context);
}

static inline void FreeDX11(RenderContext_t& context)
{
	context.m_pContext->ClearState();

	context.m_pRenderTargetView->Release();

	context.m_pSwapChain->Release();
	context.m_pContext->Release();
	context.m_pDevice->Release();
}

static inline void LoadFileInResource(int name, int type, DWORD& size, void*& data)
{
	HMODULE handle = ::GetModuleHandle(NULL);
	HRSRC rc = ::FindResource(handle, MAKEINTRESOURCE(name), MAKEINTRESOURCE(type));
	HGLOBAL rcData = ::LoadResource(handle, rc);
	size = ::SizeofResource(handle, rc);
	data = ::LockResource(rcData);
}

static inline void InitImGUI(RenderContext_t& context)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize.x = (float)context.m_iWidth;
	io.DisplaySize.y = (float)context.m_iHeight;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.IniFilename = "";

	DWORD size;
	void* fontDataTmp;
	LoadFileInResource(IDR_BCFONTFILE, FONTFILE, size, fontDataTmp);
	{
		void* fontData = malloc(size);
		memcpy(fontData, fontDataTmp, size);
		context.m_pBC961Font =
			io.Fonts->AddFontFromMemoryTTF(fontData, size, 11, 0, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());
		context.m_pDefaultFont = io.Fonts->AddFontDefault();
	}

	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 0;

	ImGui_ImplWin32_Init(context.m_hWnd);
	ImGui_ImplDX11_Init(context.m_pDevice, context.m_pContext);
}

static inline void FreeImGUI(RenderContext_t& context)
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

static inline void Render(RenderContext_t& context, Windows_t& windows)
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	{
		for (const auto& window : windows)
			window->Render(context);
	}
	ImGui::Render();

	float ClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	context.m_pContext->ClearRenderTargetView(context.m_pRenderTargetView, ClearColor);

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	context.m_pSwapChain->Present(2, 0);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd)
{
	AllocConsole();
	::ShowWindow(::GetConsoleWindow(), SW_HIDE);

	RenderContext_t renderCtx = {
		.m_hWnd = CreateDebuggerWindow(),
		.m_iWidth = g_iInitialWidth,
		.m_iHeight = g_iInitialHeight,
	};
	InitDirectX11(renderCtx);
	InitImGUI(renderCtx);

	std::vector<std::unique_ptr<IWindow>> windows;
	windows.emplace_back(std::make_unique<CMainLayoutWindow>());

	MSG msg = {};
	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) break;
		}
		else
		{
			if (g_iResizeWidth != 0 && g_iResizeHeight != 0)
			{
				CleanupRenderTarget(renderCtx);
				{
					renderCtx.m_pSwapChain->ResizeBuffers(0, g_iResizeWidth, g_iResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
					renderCtx.m_iWidth = g_iResizeWidth;
					renderCtx.m_iHeight = g_iResizeHeight;
					g_iResizeWidth = g_iResizeHeight = 0;
				}
				CreateRenderTarget(renderCtx);
			}

			Render(renderCtx, windows);
		}
	}

	FreeImGUI(renderCtx);
	FreeDX11(renderCtx);

	return 0;
}
