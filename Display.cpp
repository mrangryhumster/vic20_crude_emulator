#include "Display.h"

Display::Display(Memory* memory)
{
	m_Memory = memory;
}

Display::~Display()
{
	
}

bool Display::Initialize()
{
	//Init screen texture
	glGenTextures(1, &m_GlDisplayTexture);
	glBindTexture(GL_TEXTURE_2D, m_GlDisplayTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		DISPLAY_TEXTURE_WIDTH,//DISPLAY_CHARACTER_WIDTH * DISPLAY_SCREEN_WIDTH, 
		DISPLAY_TEXTURE_HEIGHT,//DISPLAY_CHARACTER_HEIGHT * DISPLAY_SCREEN_HEIGHT,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		nullptr);

	//Init PBO for transfer pixel data to texture
	glGenBuffers(2, m_GlDisplayPBO);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_GlDisplayPBO[0]);
	glBufferData(
		GL_PIXEL_UNPACK_BUFFER,
		DISPLAY_TEXTURE_BUFFER_SIZE,
		nullptr,
		GL_STREAM_DRAW);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_GlDisplayPBO[1]);
	glBufferData(
		GL_PIXEL_UNPACK_BUFFER,
		DISPLAY_TEXTURE_BUFFER_SIZE,
		nullptr,
		GL_STREAM_DRAW);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);



	m_DisplayBackBuffer = new uint8_t[DISPLAY_TEXTURE_BUFFER_SIZE];

	return false;
}

void Display::Destroy()
{
	delete[] m_DisplayBackBuffer;

	if (m_GlDisplayPBOMapped)
	{
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_GlDisplayPBO[0]);
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
	}
	glDeleteTextures(1, &m_GlDisplayTexture);
	glDeleteBuffers(2, m_GlDisplayPBO);
}

bool Display::UpdateDisplay()
{
	if (IsPause == false && IsImmediateMode == false)
	{
		return DisplayEmulation();
	}
	else
	{
		if (IsImmediateMode)DisplayEmulation(); //Update rasterlines
		DisplayImmediate();
		return true;
	}
}

bool Display::IsDisplayUpdated()
{
	if (IsImmediateMode) return true;
	
	bool lastState = m_DisplayUpdated;
	m_DisplayUpdated = (lastState) ? false : m_DisplayUpdated;
	return lastState;
}

GLuint Display::getDisplayTexture()
{
	return m_GlDisplayTexture;
}

GLfloat Display::getDisplayWidth()
{
	return ((GLfloat)m_DisplayWidth * DISPLAY_CHARACTER_WIDTH) / 256.f;
}

GLfloat Display::getDisplayHeight()
{
	return ((GLfloat)m_DisplayHeight * DISPLAY_CHARACTER_HEIGHT) / 256.f;
}

bool Display::DisplayEmulation()
{
	uint16_t raster_line = m_Memory->dma_read_word(0x9004) << 1 | m_Memory->dma_read_word(0x9003) >> 7;
	if (raster_line <= 312) //PAL
		raster_line+= 312 / 64;
	else
		raster_line = 0;
	m_Memory->dma_write_word(0x9003, ((raster_line & 1) << 7) | m_Memory->dma_read_word(0x9003) & 0x7f);
	m_Memory->dma_write_word(0x9004, raster_line >> 1);
	
	if (IsImmediateMode) return true; //Update timers only
	//---------------------------------------------------------------------------------------
	uint8_t color_register = m_Memory->dma_read_word(0x900F);

	uint8_t  vic_buffer_locations = m_Memory->dma_read_word(0x9005);
	uint16_t vic_memory_offset = ((m_Memory->dma_read_word(0x9002) & 0x80) ? 0x0200 : 0x0000);

	uint16_t	character_page = m_4BitAddressingTable[(vic_buffer_locations & 0x0F)];
	uint16_t	screen_page = m_4BitAddressingTable[(vic_buffer_locations >> 4)] + vic_memory_offset;
	uint16_t	color_page = 0x9400 + vic_memory_offset;

	//---------------------------------------------------------------------------------------
	m_DisplayHorizontalOffset = ((m_Memory->dma_read_word(0x9000) & 0x7F) >> 1) * 4; //Half character offset
	m_DisplayVerticalOffset = m_Memory->dma_read_word(0x9001) * 2;//Two dots offset
	m_DisplayWidth = m_Memory->dma_read_word(0x9002) & 0x7F;
	m_DisplayHeight = (m_Memory->dma_read_word(0x9003) & 0x7E) >> 1;
	//---------------------------------------------------------------------------------------
	uint32_t border_color = m_ColorTable[(color_register & 0x7)];
	uint32_t background_color = m_ColorTable[(color_register & 0xF0) >> 4];
	//---------------------------------------------------------------------------------------

	if (raster_line == 0)
	{	
		memset(m_DisplayBackBuffer, border_color, DISPLAY_TEXTURE_BUFFER_SIZE);
	}
	else if ((raster_line > 21 && raster_line < (m_DisplayHeight * 8) + 21) )
	{
		//printf("m_DisplayWidth = %d  m_DisplayHeight = %d\n", m_DisplayWidth, m_DisplayHeight);
		if (m_DisplayWidth == 0 || m_DisplayHeight == 0)
			return true;

		uint32_t character_line = ((raster_line - 22) / DISPLAY_CHARACTER_HEIGHT);
		
		for (uint32_t x = 0; x < m_DisplayWidth; x++)
		{
				uint32_t  pixelbuffer_char_offset =
					(((character_line * DISPLAY_TEXTURE_WIDTH * DISPLAY_CHARACTER_WIDTH * DISPLAY_TEXTURE_PIXEL_SIZE))) +
					((x * DISPLAY_CHARACTER_WIDTH * DISPLAY_TEXTURE_PIXEL_SIZE));

				for (uint32_t line = 0; line < DISPLAY_CHARACTER_HEIGHT; line++)
				{
					uint32_t  pixelbuffer_line_offset = pixelbuffer_char_offset + (line * DISPLAY_TEXTURE_WIDTH * DISPLAY_TEXTURE_PIXEL_SIZE);
					uint32_t* pixelbuffer_line = (uint32_t*)(m_DisplayBackBuffer + pixelbuffer_line_offset);

					uint8_t character_code = m_Memory->dma_read_word(screen_page + (character_line * (m_DisplayWidth)) + x);
					uint8_t character_part = m_Memory->dma_read_word(character_page + (character_code * (DISPLAY_CHARACTER_WIDTH)) + line);
					uint32_t character_color = m_ColorTable[m_Memory->dma_read_word(color_page + (character_line * (m_DisplayWidth)) + x) & 0x7];

					pixelbuffer_line[0] = (character_part & 0x80) ? (character_color) : (background_color);
					pixelbuffer_line[1] = (character_part & 0x40) ? (character_color) : (background_color);
					pixelbuffer_line[2] = (character_part & 0x20) ? (character_color) : (background_color);
					pixelbuffer_line[3] = (character_part & 0x10) ? (character_color) : (background_color);
					pixelbuffer_line[4] = (character_part & 0x08) ? (character_color) : (background_color);
					pixelbuffer_line[5] = (character_part & 0x04) ? (character_color) : (background_color);
					pixelbuffer_line[6] = (character_part & 0x02) ? (character_color) : (background_color);
					pixelbuffer_line[7] = (character_part & 0x01) ? (character_color) : (background_color);
				}
		}

		if (((raster_line - 22) % DISPLAY_CHARACTER_HEIGHT) == 0)
		{
			DisplayUpdateFrontBuffer();
		}
	}
	else if(raster_line == 312)
	{
		DisplayUpdateFrontBuffer();
	}
	

	return true;
}

bool Display::DisplayUpdateFrontBuffer()
{
	m_DisplayUpdated = true;

	glBindTexture(GL_TEXTURE_2D, m_GlDisplayTexture);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_GlDisplayPBO[0]);

	//---------------------------------------------------------------------------------------
	//Update PBO buffer
	glBufferData(
		GL_PIXEL_UNPACK_BUFFER,
		DISPLAY_TEXTURE_BUFFER_SIZE,
		m_DisplayBackBuffer,
		GL_STREAM_DRAW
	);
	//---------------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------------
	//Update texture from pbo
	glBindTexture(GL_TEXTURE_2D, m_GlDisplayTexture);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_GlDisplayPBO[0]);
	glTexSubImage2D(
		GL_TEXTURE_2D,
		0,
		0,
		0,
		DISPLAY_TEXTURE_WIDTH,
		DISPLAY_TEXTURE_HEIGHT,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		nullptr);
	//---------------------------------------------------------------------------------------
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	//---------------------------------------------------------------------------------------
	return false;
}

bool Display::DisplayImmediate()
{
	//---------------------------------------------------------------------------------------
	uint8_t color_register = m_Memory->dma_read_word(0x900F);

	uint8_t  vic_buffer_locations = m_Memory->dma_read_word(0x9005);
	uint16_t vic_memory_offset = ((m_Memory->dma_read_word(0x9002) & 0x80) ? 0x0200 : 0x0000);

	uint16_t	character_page = m_4BitAddressingTable[(vic_buffer_locations & 0x0F)];
	uint16_t	screen_page = m_4BitAddressingTable[(vic_buffer_locations >> 4)] + vic_memory_offset;
	uint16_t	color_page = 0x9400 + vic_memory_offset;

	//---------------------------------------------------------------------------------------
	m_DisplayHorizontalOffset = ((m_Memory->dma_read_word(0x9000) & 0x7F) >> 1) * 4; //Half character offset
	m_DisplayVerticalOffset = m_Memory->dma_read_word(0x9001) * 2;//Two dots offset
	m_DisplayWidth = m_Memory->dma_read_word(0x9002) & 0x7F;
	m_DisplayHeight = (m_Memory->dma_read_word(0x9003) & 0x7E) >> 1;
	//---------------------------------------------------------------------------------------
	uint32_t border_color = m_ColorTable[(color_register & 0x7)];
	uint32_t background_color = m_ColorTable[(color_register & 0xF0) >> 4];
	//---------------------------------------------------------------------------------------
	if (m_DisplayWidth == 0 || m_DisplayHeight == 0)
		return true;

	glBindTexture(GL_TEXTURE_2D, m_GlDisplayTexture);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_GlDisplayPBO[1]);

	//---------------------------------------------------------------------------------------
	//Reset PBO buffer
	glBufferData(
		GL_PIXEL_UNPACK_BUFFER,
		DISPLAY_TEXTURE_BUFFER_SIZE, //(size_t)m_DisplayWidth * (size_t)m_DisplayHeight * DISPLAY_CHARACTER_WIDTH * DISPLAY_PIXEL_SIZE,
		nullptr,
		GL_STREAM_DRAW
	);
	//---------------------------------------------------------------------------------------
	m_GlDisplayPBOPixels = (uint8_t*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);

	//---------------------------------------------------------------------------------------

	//Border color
	memset(m_GlDisplayPBOPixels, border_color, DISPLAY_TEXTURE_BUFFER_SIZE);
	for (uint32_t y = 0; y < m_DisplayHeight; y++)
	{
		for (uint32_t x = 0; x < m_DisplayWidth; x++)
		{
			uint32_t  pixelbuffer_char_offset =
				(((y * DISPLAY_TEXTURE_WIDTH * DISPLAY_CHARACTER_WIDTH * DISPLAY_TEXTURE_PIXEL_SIZE))) +
				((x * DISPLAY_CHARACTER_WIDTH * DISPLAY_TEXTURE_PIXEL_SIZE));

			for (uint32_t line = 0; line < DISPLAY_CHARACTER_HEIGHT; line++)
			{
				uint32_t  pixelbuffer_line_offset = pixelbuffer_char_offset + (line * DISPLAY_TEXTURE_WIDTH * DISPLAY_TEXTURE_PIXEL_SIZE);
				uint32_t* pixelbuffer_line = (uint32_t*)(m_GlDisplayPBOPixels + pixelbuffer_line_offset);

				uint8_t character_code = m_Memory->dma_read_word(screen_page + (y * (m_DisplayWidth)) + x);
				uint8_t character_part = m_Memory->dma_read_word(character_page + (character_code * (DISPLAY_CHARACTER_WIDTH)) + line);
				uint32_t character_color = m_ColorTable[m_Memory->dma_read_word(color_page + (y * (m_DisplayWidth)) + x) & 0x7];

				pixelbuffer_line[0] = (character_part & 0x80) ? (character_color) : (background_color);
				pixelbuffer_line[1] = (character_part & 0x40) ? (character_color) : (background_color);
				pixelbuffer_line[2] = (character_part & 0x20) ? (character_color) : (background_color);
				pixelbuffer_line[3] = (character_part & 0x10) ? (character_color) : (background_color);
				pixelbuffer_line[4] = (character_part & 0x08) ? (character_color) : (background_color);
				pixelbuffer_line[5] = (character_part & 0x04) ? (character_color) : (background_color);
				pixelbuffer_line[6] = (character_part & 0x02) ? (character_color) : (background_color);
				pixelbuffer_line[7] = (character_part & 0x01) ? (character_color) : (background_color);
			}
		}
	}
	//---------------------------------------------------------------------------------------
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

	//Update texture from pbo
	glBindTexture(GL_TEXTURE_2D, m_GlDisplayTexture);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_GlDisplayPBO[1]);
	glTexSubImage2D(
		GL_TEXTURE_2D,
		0,
		0,
		0,
		DISPLAY_TEXTURE_WIDTH,
		DISPLAY_TEXTURE_HEIGHT,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		nullptr);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	//---------------------------------------------------------------------------------------
	return true;
}
