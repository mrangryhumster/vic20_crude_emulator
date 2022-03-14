#pragma once

#include "GL/gl3w.h"
#include "Memory.h"


#define DISPLAY_TEXTURE_WIDTH 256
#define DISPLAY_TEXTURE_HEIGHT 256
#define DISPLAY_TEXTURE_PIXEL_SIZE 4
#define DISPLAY_TEXTURE_BUFFER_SIZE DISPLAY_TEXTURE_WIDTH * DISPLAY_TEXTURE_HEIGHT * DISPLAY_TEXTURE_PIXEL_SIZE


#define DISPLAY_SCREEN_WIDTH 22
#define DISPLAY_SCREEN_HEIGHT 23
#define DISPLAY_CHARACTER_WIDTH 8
#define DISPLAY_CHARACTER_HEIGHT 8


//(DISPLAY_CHARACTER_WIDTH * DISPLAY_SCREEN_WIDTH) * (DISPLAY_CHARACTER_HEIGHT * DISPLAY_SCREEN_HEIGHT) * DISPLAY_PIXEL_SIZE

class Display
{
	public:
		Display(Memory* memory);
		~Display();

		bool Initialize();
		void Destroy();
		bool UpdateDisplay();

		bool IsDisplayUpdated();


		GLuint  getDisplayTexture();
		GLfloat getDisplayWidth();
		GLfloat getDisplayHeight();

#pragma region -- Control EndPoints --
		bool IsPause;
		bool IsImmediateMode;
		bool IsError;
#pragma endregion

	private:

		bool DisplayEmulation();
		bool DisplayUpdateFrontBuffer();
		bool DisplayImmediate();

		Memory* m_Memory;
		//--------------------------------
		GLuint		m_GlDisplayTexture;
		
		GLuint		m_GlDisplayPBO[2];
		GLboolean	m_GlDisplayPBOSwap;
		GLboolean   m_GlDisplayPBOMapped;
		uint8_t*	m_GlDisplayPBOPixels;
		//--------------------------------
		uint8_t*	m_DisplayBackBuffer;
		//--------------------------------
		bool		m_DisplayUpdated;
		//--------------------------------

		uint32_t m_DisplayWidth;
		uint32_t m_DisplayHeight;
		uint32_t m_DisplayHorizontalOffset;
		uint32_t m_DisplayVerticalOffset;
		//--------------------------------
		const uint16_t m_4BitAddressingTable[16] =
		{
			0x8000,0x8400,0x8800,0x8C00,
			0x9000,0x9400,0x9800,0x9C00,
			0x0000,0x0400,0x0800,0x0C00,
			0x1000,0x1400,0x1800,0x1C00
		};

		const uint32_t m_ColorTable[16] =
		{
			0xFF000000,
			0xFFFFFFFF,

			0xFF211FB6,
			0xFFFFF04D,
			0xFFFF3FB4,
			0xFF37E244,
			0xFFF7570F,
			0xFF1BD7DC,
			0xFF0054CA,
			0xFF72B0E9,
			0xFF9392E7,
			0xFFFDF79A,
			0xFFFF9FE0,
			0xFF93E48F,
			0xFFFF9082,
			0xFF85DEE5,
		};
};

