#pragma once

#define  _CRT_SECURE_NO_WARNINGS
//----------------------------------------
#pragma warning(push, 0)
#include "GL\gl3w.h"

#define SDL_MAIN_HANDLED
#include "SDL.h"

#include "imgui.h"
#include "backends\imgui_impl_sdl.h"
#include "backends\imgui_impl_opengl3.h"
#include "imgui_memory_editor.h"
#pragma warning(pop)
//----------------------------------------
#include "Defaults.h"
#include "Characters.h"
#include "Basic.h"
#include "Kernel.h"
#include "Programm.h"
#include "Sample.h"
#include "Snapshots.h"

#include "Memory.h"
#include "MOS6502.h"
#include "Display.h"
#include "Input.h"
//----------------------------------------

#define WINDOW_SIZE_WIDTH 640*2
#define WINDOW_SIZE_HEIGHT 480*2

//----------------------------------------
class VIC20Emulator
{
public:
	VIC20Emulator();
	~VIC20Emulator();

	int Run();

protected:

	bool Initialize();
	bool MainLoop();


	//--------------------------------


	//--------------------------------
	SDL_Window*		m_SDLWindow;
	SDL_GLContext	m_SDLGlContext;
	//--------------------------------
	Memory*  m_Memory;
	MOS6502* m_Cpu;
	Display* m_Display;
	Input*	 m_Input;
	//--------------------------------
	//ImGui Widgets
	MemoryEditor m_ImGuiWidgetMemoryEditor;
	//--------------------------------
	bool m_guiWindowedMode;
	bool m_guiMemoryEditor;
	//--------------------------------

};

