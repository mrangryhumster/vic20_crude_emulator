#pragma once

#define _CRT_SECURE_NO_WARNINGS

#define CPU_PRINT_DISASSEBLE
//#define CPU_PRINT_MEMORYACCESS
//#define CPU_PRINT_MEMORYACCESS_OPFETCH

#define CPU_STOP_ON_HANGUP_GENERAL

#include <cstdint>
#include "MOS6502Macro.h"
#include "Memory.h"
class MOS6502
{
public:
	MOS6502(Memory* ram, uint16_t stack_offset);
	~MOS6502();

	void reset();
	
	bool clock();
	bool execute();
	
	void interrupt();
	void disassemble(char* buffer, uint16_t address,bool add_registers,bool force);

#pragma region -- Registers -- 
	struct Registers
	{
		enum class StatusFlags : uint8_t
		{
			NegativeBit = 0b10000000,
			OverflowBit = 0b01000000,
			UnusedBit = 0b00100000,
			BreakBit = 0b00010000,
			DecimalBit = 0b00001000,
			InterruptBit = 0b00000100,
			ZeroBit = 0b00000010,
			CarryBit = 0b00000001,
		};

		uint16_t PC;	//	 program counter(16 bit)
		uint8_t	 AC;	//	 accumulator(8 bit)
		uint8_t  XR;	// X register	(8 bit)
		uint8_t  YR;	// Y register	(8 bit)
		uint8_t  SR;	//	 status register[NV - BDIZC](8 bit)
		uint8_t  SP;	//	 stack pointer(8 bit)

		void setFlag(StatusFlags flag, bool val) { (val) ? SR |= static_cast<uint8_t>(flag) : SR &= ~static_cast<uint8_t>(flag); }
		bool getFlag(StatusFlags flag) { return (SR & static_cast<uint8_t>(flag)); }
	} m_Registers;
#pragma endregion

#pragma region -- CPU Pipeline --
	struct Pipeline
	{
		bool	 Break;
		bool     Interrupt;

		uint8_t  LastInstructionOpCode;
		uint16_t LastInstructionAddress;
		uint32_t LastInstructionCycle;

		uint32_t GlobalClockCycle;
		uint32_t InstructionCycle;
		uint32_t InstructionCounter;
		
	}m_Pipeline;
#pragma endregion

	bool Disasm;

#pragma region -- Data Bus --
	struct DataBus
	{
		enum class AddressingMode : uint8_t
		{
			Undefined,
			Accumulator,
			Absolute,
			AbsoluteXIndexed,
			AbsoluteYIndexed,
			Immediate,
			Implied,
			Indirect,
			XIndexedIndirect,
			IndirectYIndexed,
			Relative,
			Zeropage,
			ZeropageXIndexed,
			ZeropageYIndexed,
		};
		AddressingMode	AddressingMode;
		uint16_t		Address;
		uint8_t			Data;
		uint16_t		Intermediate;
	}m_DataBus;

	void data_read();
	void data_write();

#pragma endregion

#pragma region -- Control EndPoints --
	bool IsPause;
	bool IsError;
#pragma endregion


protected:
	Memory* m_MemoryPtr;
	uint16_t m_StackOffset;


	

	#pragma region -- Instrtuctions --

	//instructions
	void execute_adc(); // add with carry
	void execute_and(); // and (with accumulator)
	void execute_asl(); // arithmetic shift left
	void execute_bcc(); // branch on carry clear
	void execute_bcs(); // branch on carry set
	void execute_beq(); // branch on equal (zero set)
	void execute_bit(); // bit test
	void execute_bmi(); // branch on minus (negative set)
	void execute_bne(); // branch on not equal (zero clear)
	void execute_bpl(); // branch on plus (negative clear)
	void execute_brk(); // break / interrupt
	void execute_bvc(); // branch on overflow clear
	void execute_bvs(); // branch on overflow set
	void execute_clc(); // clear carry
	void execute_cld(); // clear decimal
	void execute_cli(); // clear interrupt disable
	void execute_clv(); // clear overflow
	void execute_cmp(); // compare (with accumulator)
	void execute_cpx(); // compare with x
	void execute_cpy(); // compare with y
	void execute_dec(); // decrement
	void execute_dex(); // decrement x
	void execute_dey(); // decrement y
	void execute_eor(); // exclusive or (with accumulator)
	void execute_inc(); // increment
	void execute_inx(); // increment x
	void execute_iny(); // increment y
	void execute_jmp(); // jump
	void execute_jsr(); // jump subroutine
	void execute_lda(); // load accumulator
	void execute_ldx(); // load x
	void execute_ldy(); // load y
	void execute_lsr(); // logical shift right
	void execute_nop(); // no operation
	void execute_ora(); // or with accumulator
	void execute_pha(); // push accumulator
	void execute_php(); // push processor status (sr)
	void execute_pla(); // pull accumulator
	void execute_plp(); // pull processor status (sr)
	void execute_rol(); // rotate left
	void execute_ror(); // rotate right
	void execute_rti(); // return from interrupt
	void execute_rts(); // return from subroutine
	void execute_sbc(); // subtract with carry
	void execute_sec(); // set carry
	void execute_sed(); // set decimal
	void execute_sei(); // set interrupt disable
	void execute_sta(); // store accumulator
	void execute_stx(); // store x
	void execute_sty(); // store y
	void execute_tax(); // transfer accumulator to x
	void execute_tay(); // transfer accumulator to y
	void execute_tsx(); // transfer stack pointer to x
	void execute_txa(); // transfer x to accumulator
	void execute_txs(); // transfer x to stack pointer
	void execute_tya(); // transfer y to accumulator 
	void execute_xxx(); //unknown opcode
	#pragma endregion
};

