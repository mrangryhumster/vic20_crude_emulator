#include "VIC20Emulator.h"



VIC20Emulator::VIC20Emulator()
{
}

VIC20Emulator::~VIC20Emulator()
{

	m_Display->Destroy();

	SDL_GL_DeleteContext(m_SDLGlContext);
	SDL_DestroyWindow(m_SDLWindow);
	SDL_Quit();
}

int VIC20Emulator::Run()
{
	if (this->Initialize() == false)
	{
		printf("Initialization failed\n");
		return 1;
	}

	if (this->MainLoop() == false)
	{
		printf("Smth broke due emulation\n");
		return 2;
	}

	return 0;
}

bool VIC20Emulator::Initialize()
{
	//------------------------------------------------------------------------------------------------------------
	// 	SDL, gl3w and Dear ImGui initialization and basic setup 
	//------------------------------------------------------------------------------------------------------------
	SDL_SetMainReady();
	//------------------------------------------------------
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
	{
		printf("Failed to init SDL\n");
		return false;
	}
	//------------------------------------------------------
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	//------------------------------------------------------
	m_SDLWindow = SDL_CreateWindow(
		"MainWindow",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		WINDOW_SIZE_WIDTH, WINDOW_SIZE_HEIGHT,
		(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI));

	m_SDLGlContext = SDL_GL_CreateContext(m_SDLWindow);;

	SDL_GL_MakeCurrent(m_SDLWindow, m_SDLGlContext);
	//SDL_GL_SetSwapInterval(1); // Enable vsync
	//------------------------------------------------------

	//------------------------------------------------------
	if (gl3wInit())
	{
		printf("Failed to init gl3w\n");
		return false;
	}
	//------------------------------------------------------

	//------------------------------------------------------
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	//------------------------------------------------------
	ImGui::StyleColorsDark();
	//------------------------------------------------------
	ImGui_ImplSDL2_InitForOpenGL(m_SDLWindow, m_SDLGlContext);
	ImGui_ImplOpenGL3_Init("#version 150");

	//------------------------------------------------------------------------------------------------------------
	// Emulator memory and cpu initialization and setup
	//------------------------------------------------------------------------------------------------------------
	m_Memory = new Memory();
	m_Cpu = new MOS6502(m_Memory, 0x0100);
	m_Input = new Input(m_Cpu, m_Memory);
	m_Display = new Display(m_Memory);
	//------------------------------------------------------
	memcpy(m_Memory->data() + 0x8000, VIC20_CHARACTERS, 4096);
	memcpy(m_Memory->data() + 0xC000, VIC20_BASIC, 8192);
	memcpy(m_Memory->data() + 0xe000, VIC20_KERNAL_901486_07, 8192);
	memcpy(m_Memory->data() + (0xa000-2), VIC20_SAMPLE_3_ROM, sizeof(VIC20_SAMPLE_3_ROM) );
	//------------------------------------------------------
	m_Cpu->reset();
	m_Display->Initialize();
	m_Input->Initialize();
	//------------------------------------------------------------------------------------------------------------
	// Misc setup
	//------------------------------------------------------------------------------------------------------------
	m_ImGuiWidgetMemoryEditor.Cols = 16;

	//------------------------------------------------------------------------------------------------------------
	//PrimeCPU
	for (int i = 0; i < 20000; i++)
	{
		m_Cpu->clock();
	}
	//------------------------------------------------------------------------------------------------------------
	return true;
}



bool VIC20Emulator::MainLoop()
{

	SDL_Event event;
	ImGuiIO& ImGuiIO = ImGui::GetIO();
	bool IsRunning = true, IsCpuRunning = true, IsDisplayRunning = true;
	bool DoClock = false, DoClockState = false, GlobalSync = true;
	int CPUPerFrame = 1;
	int GPUUpdatesPerFrame = 1, GPUFrameSync = 60, GPUFrameSyncCounter = 0;
	int TimersUpdatesPerFrame = 1, TimersFrameSync = 60, TimersFrameSyncCounter = 0;
	int ioPaddle = 0;
	while (IsRunning)
	{

		do
		{
			if (GlobalSync)
			{
				if (CPUPerFrame > 128) CPUPerFrame = 128;
				if (GPUUpdatesPerFrame > 128) GPUUpdatesPerFrame = 128;
				if (TimersUpdatesPerFrame > 128) TimersUpdatesPerFrame = 128;
			}
			//CPU Step
			for (int i = 0; i < CPUPerFrame; i++)
			{
				IsCpuRunning = m_Cpu->clock();
			}
			//************************************
			m_Display->IsPause = m_Cpu->IsPause;
			m_Input->IsPause = m_Cpu->IsPause;
			//************************************
			if (m_Input->IsImmediateMode)
			{
				if ((--TimersFrameSyncCounter) <= 0)
				{
					m_Input->UpdateInput();
					TimersFrameSyncCounter = TimersFrameSync;
				}
			}
			else
			{
				for (int i = 0; i < TimersUpdatesPerFrame; i++)
				{
					m_Input->UpdateInput();
				}
			}
			//************************************
			if (m_Display->IsImmediateMode)
			{
				if ((--GPUFrameSyncCounter) <= 0)
				{
					IsDisplayRunning = m_Display->UpdateDisplay();
					GPUFrameSyncCounter = GPUFrameSync;
				}
			}
			else
			{
				for (int i = 0; i < GPUUpdatesPerFrame; i++)
				{
					IsDisplayRunning = m_Display->UpdateDisplay();
				}
			}
			//************************************
			if (DoClock)
			{
				if (DoClockState)
				{
					m_Cpu->IsPause = false;
					m_Input->IsPause = false;
					m_Display->IsPause = false;

					m_Cpu->clock();
					m_Input->UpdateInput();
					m_Display->UpdateDisplay();

					m_Cpu->IsPause = true;
					m_Input->IsPause = true;
					m_Display->IsPause = true;

					DoClock = false;
				}
			}
			else
			{
				DoClockState = true;
			}

		} while (!m_Display->IsDisplayUpdated() && GlobalSync && !m_Cpu->IsPause);
		//************************************
		//Pool SDL events
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);

			if (event.type == SDL_KEYDOWN)
			{
				m_Input->setKeyPressed(event.key.keysym.sym, true);
				m_Input->setJoystick(event.key.keysym.sym, true);
			}
			else if (event.type == SDL_KEYUP)
			{
				m_Input->setKeyPressed(event.key.keysym.sym, false);
				m_Input->setJoystick(event.key.keysym.sym, false);
			}

			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)
			{
				IsRunning = false;
			}
		}
		m_Input->setPaddle(ioPaddle, true);
		//*************************************************************************
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(m_SDLWindow);
		ImGui::NewFrame();
		//*************************************************************************
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("Options"))
			{
				ImGui::MenuItem("Windowed Mode", NULL, &m_guiWindowedMode);
				ImGui::MenuItem("Global Synchronisation", NULL, &GlobalSync);
				ImGui::MenuItem("Immediate Raster", NULL, &m_Display->IsImmediateMode);
				ImGui::MenuItem("Immediate Timers", NULL, &m_Input->IsImmediateMode);
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Tools"))
			{
				ImGui::MenuItem("Memory Editor", NULL, &m_guiMemoryEditor);
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Debug"))
			{
				ImGui::MenuItem("Pause CPU", NULL, &m_Cpu->IsPause);

				ImGui::MenuItem("Disassemble", NULL, &m_Cpu->Disasm);
				if (ImGui::MenuItem("Reset CPU"))
				{
					m_Cpu->reset();
				};
				if (ImGui::MenuItem("Wipe RAM"))
				{
					memset(m_Memory->data(), 0x00, 0x10000);
				}
				if (ImGui::MenuItem("Reload RAM"))
				{
					memset(m_Memory->data(), 0x00, 0x10000);
					memcpy(m_Memory->data() + 0x8000, VIC20_CHARACTERS, 4096);
					memcpy(m_Memory->data() + 0xC000, VIC20_BASIC, 8192);
					memcpy(m_Memory->data() + 0xe000, VIC20_KERNAL_901486_07, 8192);
				}
				if (ImGui::MenuItem("Reset to BASIC"))
				{
					memset(m_Memory->data(), 0x00, 0x10000);
					memcpy(m_Memory->data() + 0x8000, VIC20_CHARACTERS, 4096);
					memcpy(m_Memory->data() + 0xC000, VIC20_BASIC, 8192);
					memcpy(m_Memory->data() + 0xe000, VIC20_KERNAL_901486_07, 8192);
					m_Cpu->reset();
				}
				if (ImGui::MenuItem("Reset to Functional TEST"))
				{
					memcpy(m_Memory->data(), VIC20_6502_FUNCTIONAL_TEST, sizeof(VIC20_6502_FUNCTIONAL_TEST));
					m_Cpu->reset();
					m_Cpu->m_Registers.PC = 0x0400;
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
		//*************************************************************************
		ImGuiWindowFlags vic20DisplayFlags;
		if (m_guiWindowedMode)
		{
			vic20DisplayFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus;
		}
		else
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();

			vic20DisplayFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
		}

		if (ImGui::Begin("VIC20Display", nullptr, vic20DisplayFlags))
		{
			ImVec2 windowSize = ImGui::GetWindowSize();
			windowSize.x -= ImGui::GetStyle().WindowPadding.x * 2;
			windowSize.y -= ImGui::GetStyle().WindowPadding.y * 5;

			ImGui::Image((ImTextureID)m_Display->getDisplayTexture(),
				windowSize,
				ImVec2(0, 0), ImVec2(m_Display->getDisplayWidth(), m_Display->getDisplayHeight()));


		}	ImGui::End();
		//*************************************************************************
		if (ImGui::Begin("DEBUG Control"))
		{
			DoClock = ImGui::Button("One Step");
			ImGui::SliderInt("CPU Updates", &CPUPerFrame, 1, (GlobalSync) ? 128 : 1024);
			if (m_Display->IsImmediateMode)
				ImGui::SliderInt("FPS GPU SYNC", &GPUFrameSync, 1, 512);
			else
				ImGui::SliderInt("GPU Updates", &GPUUpdatesPerFrame, 1, (GlobalSync) ? 128 : 1024);

			if (m_Input->IsImmediateMode)
				ImGui::SliderInt("FPS TMR SYNC", &TimersFrameSync, 1, 512);
			else
				ImGui::SliderInt("TMR Updates", &TimersUpdatesPerFrame, 1, (GlobalSync) ? 128 : 1024);
		}	ImGui::End();
		//-----------------------------------------------------
		if (m_guiMemoryEditor)
		{
			m_ImGuiWidgetMemoryEditor.DrawWindow("Memory Editor", m_Memory->data(), 0x10000);
		}
		//*************************************************************************

		ImGui::Render();
		glViewport(0, 0, (int)ImGuiIO.DisplaySize.x, (int)ImGuiIO.DisplaySize.y);
		glClearColor(0.33f, 0.33f, 0.66f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(m_SDLWindow);
	}
	return true;
}
