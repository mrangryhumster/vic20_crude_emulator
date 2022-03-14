#pragma once

#include "SDL.h"
#include "Memory.h"
#include "MOS6502.h"

class Input
{
public:
	Input(MOS6502* cpu,Memory* memory);
	~Input();

	bool Initialize();
	bool UpdateInput();


	void setKeyPressed(uint32_t code, bool state);
	void setJoystick(uint32_t code,bool state);
	void setPaddle(uint8_t paddle,bool is_first_user);

#pragma region -- Control EndPoints --
	bool IsPause;
	bool IsImmediateMode;
	bool IsError;
#pragma endregion

private:

	void UpdateTimers();
	void UpdateKeyboard();

	MOS6502* m_Cpu;
	Memory* m_Memory;

	uint16_t m_KeyPressed;
	bool m_ShiftPressed;
	bool m_CBMPressed;
	bool 	 m_Joystick[6];

	//VIA 1
	class VIA1
	{
	public:
		static const uint16_t PortB = 0x9110;
		static const uint16_t PortA = 0x9111;
		static const uint16_t PortB_DDR = 0x9112;
		static const uint16_t PortA_DDR = 0x9113;
		static const uint16_t Timer1LOByteLatch = 0x9114;
		static const uint16_t Timer1HIByteLatch = 0x9115;
		static const uint16_t Timer1LOByte = 0x9116;
		static const uint16_t Timer1HIByte = 0x9117;
		static const uint16_t Timer2LOByte = 0x9118;
		static const uint16_t Timer2HIByte = 0x9119;
		static const uint16_t ShiftRegister = 0x911A;
		static const uint16_t AuxiliaryRegister = 0x911B;
		static const uint16_t PeripheralRegister = 0x911C;
		static const uint16_t InterruptFlag = 0x911D;
		static const uint16_t InterruptEnable = 0x911E;
		static const uint16_t PortAOutput = 0x911F;
	};
	//VIA 2
	class VIA2
	{
	public:
		static const uint16_t PortB = 0x9120;
		static const uint16_t PortA = 0x9121;
		static const uint16_t PortB_DDR = 0x9122;
		static const uint16_t PortA_DDR = 0x9123;
		static const uint16_t Timer1LOByteLatch = 0x9124;
		static const uint16_t Timer1HIByteLatch = 0x9125;
		static const uint16_t Timer1LOByte = 0x9126;
		static const uint16_t Timer1HIByte = 0x9127;
		static const uint16_t Timer2LOByte = 0x9128;
		static const uint16_t Timer2HIByte = 0x9129;
		static const uint16_t ShiftRegister = 0x912A;
		static const uint16_t AuxiliaryRegister = 0x912B;
		static const uint16_t PeripheralRegister = 0x912C;
		static const uint16_t InterruptFlag = 0x912D;
		static const uint16_t InterruptEnable = 0x912E;
		static const uint16_t PortAOutput = 0x912F;
	};

	uint16_t ConvertKeyboardKeycode(SDL_Keycode sdl_keycode)
	{
		//SDL TO VIC20
		switch (sdl_keycode) 
		{
		case SDLK_2: return 0x7FFE; break;
		case SDLK_q: return 0xBFFE; break;
		case SDLK_RCTRL: return 0xDFFE; break; // CBM
		case SDLK_SPACE: return 0xEFFE; break;

		case SDLK_LCTRL: return 0xFBFE; break;
		case SDLK_LEFT: return 0xFDFE; break;
		case SDLK_1: return 0xFEFE; break;

		// FD row
		case SDLK_4: return 0x7FFD; break;
		case SDLK_e: return 0xBFFD; break;
		case SDLK_s: return 0xDFFD; break;
		case SDLK_z: return 0xEFFD; break;
		case SDLK_LSHIFT: return 0xF7FD; break;
		case SDLK_a: return 0xFBFD; break;
		case SDLK_w: return 0xFDFD; break;
		case SDLK_3: return 0xFEFD; break;

		// FB row
		case SDLK_6: return 0x7FFB; break;
		case SDLK_t: return 0xBFFB; break;
		case SDLK_f: return 0xDFFB; break;
		case SDLK_c: return 0xEFFB; break;
		case SDLK_x: return 0xF7FB; break;
		case SDLK_d: return 0xFBFB; break;
		case SDLK_r: return 0xFDFB; break;
		case SDLK_5: return 0xFEFB; break;

		// F7 row
		case SDLK_8: return 0x7FF7; break;
		case SDLK_u: return 0xBFF7; break;
		case SDLK_h: return 0xDFF7; break;
		case SDLK_b: return 0xEFF7; break;
		case SDLK_v: return 0xF7F7; break;
		case SDLK_g: return 0xFBF7; break;
		case SDLK_y: return 0xFDF7; break;
		case SDLK_7: return 0xFEF7; break;

		// EF row
		case SDLK_0: return 0x7FEF; break;
		case SDLK_o: return 0xBFEF; break;
		case SDLK_k: return 0xDFEF; break;
		case SDLK_m: return 0xEFEF; break;
		case SDLK_n: return 0xF7EF; break;
		case SDLK_j: return 0xFBEF; break;
		case SDLK_i: return 0xFDEF; break;
		case SDLK_9: return 0xFEEF; break;

		// DF row
		case SDLK_MINUS: return 0x7FDF; break;
		case SDLK_MAIL: return 0xBFDF; break;
		case SDLK_COLON: return 0xDFDF; break;
		case SDLK_PERIOD: return 0xEFDF; break;
		case SDLK_COMMA: return 0xF7DF; break;
		case SDLK_l: return 0xFBDF; break;
		case SDLK_p: return 0xFDDF; break;
		case SDLK_PLUS: return 0xFEDF; break;

		// BF row
		case SDLK_HOME: return 0x7FBF; break;
		case SDLK_UP: return 0xBFBF; break;
		case SDLK_RALT: return 0xDFBF; break;
		case SDLK_RSHIFT: return 0xEFBF; break;
		case SDLK_BACKSLASH: return 0xF7BF; break;
		case SDLK_SEMICOLON: return 0xFBBF; break;
		case SDLK_ASTERISK: return 0xFDBF; break;
		case SDLK_INSERT: return 0xFEBF; break;

		// 7F row
		case SDLK_F7: return 0x7F7F; break;
		case SDLK_F5: return 0xBF7F; break;
		case SDLK_F3: return 0xDF7F; break;
		case SDLK_F1: return 0xEF7F; break;
		case SDLK_DOWN: return 0xF77F; break;
		case SDLK_RIGHT: return 0xFB7F; break;
		case SDLK_RETURN: return 0xFD7F; break;
		case SDLK_BACKSPACE: return 0xFE7F; break;
		default: return 0x0000; break;
		}
	}
	uint16_t ConvertJoystickKeycode(SDL_Keycode sdl_keycode)
	{
		
		//SDL TO VIC20
		switch (sdl_keycode)
		{
		case SDLK_KP_5:	return 0; // FIRE
		case SDLK_KP_0:	return 0; // FIRE
		case SDLK_KP_8:	return 1; // UP
		case SDLK_KP_6:	return 2; // RIGHT
		case SDLK_KP_2:	return 3; // DOWN
		case SDLK_KP_4:	return 4; // LEFT
		default: return 0x0005; break;
		}
	}
};

