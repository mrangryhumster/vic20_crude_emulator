#include "Memory.h"
#include <ctype.h>

Memory::Memory()
{
	m_MemoryPtr = new uint8_t[0x10000];
	memset(m_MemoryPtr, 0, 0x10000);
}

Memory::~Memory()
{
	delete[] m_MemoryPtr;
}

uint8_t Memory::read(uint16_t address)
{
	/*
	if (address >= 0x0400 && address <= 0x07ff) // EXPANSION RAM 1
	{
		return 0;
	}
	if (address >= 0x0800 && address <= 0x0bff) // EXPANSION RAM 2
	{
		return 0;
	}
	if (address >= 0x0c00 && address <= 0x0fff) // EXPANSION RAM 3
	{
		return 0;
	}

	if (address >= 0x2000 && address <= 0x3fff) // EXPANSION BLK 1
	{
		return 0;
	}
	if (address >= 0x4000 && address <= 0x5fff) // EXPANSION BLK 2
	{
		return 0;
	}
	if (address >= 0x6000 && address <= 0x7fff) // EXPANSION BLK 3
	{
		return 0;
	}
	if (address >= 0xa000 && address <= 0xbfff) // EXPANSION BLK 5
	{
		return 0;
	}
	*/
	if (address >= 0x9000 && address <= 0x9fff) // IO
	{
		//printf("IO ACCESS - READ [%.4x] = $%.2x\n", address, *(uint8_t*)(m_MemoryPtr + address));
	}
	return *(uint8_t*)(m_MemoryPtr + address);
}

void Memory::write(uint16_t address, uint8_t byte)
{
	/*
	if (address >= 0x0400 && address <= 0x07ff) // EXPANSION RAM 1
	{
		return;
	}
	if (address >= 0x0800 && address <= 0x0bff) // EXPANSION RAM 2
	{
		return;
	}
	if (address >= 0x0c00 && address <= 0x0fff) // EXPANSION RAM 3
	{
		return;
	}

	if (address >= 0x2000 && address <= 0x3fff) // EXPANSION BLK 1
	{
		return;
	}
	if (address >= 0x4000 && address <= 0x5fff) // EXPANSION BLK 2
	{
		return;
	}
	if (address >= 0x6000 && address <= 0x7fff) // EXPANSION BLK 3
	{
		return;
	}
	if (address >= 0xa000 && address <= 0xbfff) // EXPANSION BLK 5
	{
		return;
	}
	*/
	if (address >= 0x8000 && address <= 0x8fff) // Character ROM 
	{
		return;
	}
	if (address >= 0xC000 && address <= 0xDfff) // BASIC ROM
	{
		return;
	}
	if (address >= 0xC000 && address <= 0xDfff) // KERNAL ROM
	{
		return;
	}
	

	(*(uint8_t*)(m_MemoryPtr + address)) = byte;
}

uint8_t Memory::dma_read_word(uint16_t address)
{
	return *(uint8_t*)(m_MemoryPtr + address);
}

uint16_t Memory::dma_read_dword(uint16_t address)
{
	return *(uint16_t*)(m_MemoryPtr + address);
}

void Memory::dma_write_word(uint16_t address, uint8_t byte)
{
	(*(uint8_t*)(m_MemoryPtr + address)) = byte;
}

void Memory::dma_write_dword(uint16_t address, uint16_t word)
{
	(*(uint16_t*)(m_MemoryPtr + address)) = word;
}


void Memory::dump(const char* filepath)
{
	FILE* dumpFile = fopen(filepath, "w");
	for (int lines = 0; lines < (0xFFFF / MEMORYDUMP_DEFAULT_COLUMNS); lines++)
	{
		fprintf(dumpFile,"\t:%.4x\t", lines * MEMORYDUMP_DEFAULT_COLUMNS);
		for (int columns = 0; columns < MEMORYDUMP_DEFAULT_COLUMNS; columns++)
		{
			fprintf(dumpFile, "%.2x ", m_MemoryPtr[(lines * MEMORYDUMP_DEFAULT_COLUMNS) + columns]);
		}
		fprintf(dumpFile, "\t");
		for (int columns = 0; columns < MEMORYDUMP_DEFAULT_COLUMNS; columns++)
		{
			if (isprint(m_MemoryPtr[(lines * MEMORYDUMP_DEFAULT_COLUMNS) + columns]))
				fprintf(dumpFile, "%c", m_MemoryPtr[(lines * MEMORYDUMP_DEFAULT_COLUMNS) + columns]);
			else
				fprintf(dumpFile, ".");
		}
		fprintf(dumpFile, "\n");
	}
	fclose(dumpFile);
}

uint8_t* Memory::data()
{
	return m_MemoryPtr;
}
