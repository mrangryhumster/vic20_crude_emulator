#pragma once

#define _CRT_SECURE_NO_WARNINGS

#define MEMORYDUMP_DEFAULT_FILE		"memory_dump.dat"
#define MEMORYDUMP_DEFAULT_COLUMNS	60

#include <cstdint>
#include <memory>

class Memory
{
public:
	Memory();
	~Memory();
	

	//Tracked memory access
	uint8_t		read(uint16_t address);
	void		write(uint16_t address, uint8_t byte);

	//Untracked memory access
	uint8_t		dma_read_word(uint16_t address);
	uint16_t	dma_read_dword(uint16_t address);
	void		dma_write_word(uint16_t address, uint8_t byte);
	void		dma_write_dword(uint16_t address, uint16_t word);

	//Pointer to array
	uint8_t*	data();

	//Debug stuff
	void		dump(const char* filepath);


protected:
	uint8_t* m_MemoryPtr;
	size_t m_MemorySize;
};

