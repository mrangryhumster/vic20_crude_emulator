#pragma once



#pragma region -- BitShift Helpers --
#define BS_GETLO(DATA) ((DATA & 0xFF00) >> 8)
#define BS_GETHI(DATA) ((DATA & 0x00FF))
#pragma endregion

#pragma region -- Register Helpers --
#define SR_NEGATIVEBIT MOS6502::Registers::StatusFlags::NegativeBit
#define SR_OVERFLOWBIT MOS6502::Registers::StatusFlags::OverflowBit
#define SR_UNUSEDBIT MOS6502::Registers::StatusFlags::UnusedBit
#define SR_BREAKBIT MOS6502::Registers::StatusFlags::BreakBit
#define SR_DECIMALBIT MOS6502::Registers::StatusFlags::DecimalBit
#define SR_INTERRUPTBIT MOS6502::Registers::StatusFlags::InterruptBit
#define SR_ZEROBIT MOS6502::Registers::StatusFlags::ZeroBit
#define SR_CARRYBIT MOS6502::Registers::StatusFlags::CarryBit
#pragma endregion

#pragma region -- Pipeline Helpers --
#define PL_START_PIPELINE(FIRST) if(m_Pipeline.InstructionCycle == FIRST) { m_Pipeline.InstructionCycle = FIRST + 1;
#define PL_STEP_PIPELINE(STAGE) } else if(m_Pipeline.InstructionCycle == STAGE) { m_Pipeline.InstructionCycle += 1;
#define PL_STOP_PIPELINE() m_Pipeline.InstructionCycle = 0; }
#define PL_BREAK_PIPELINE() m_Pipeline.InstructionCycle = 0;
#pragma endregion

#pragma region -- DataBus Helpers --
#define AM_UNDEFINED MOS6502::DataBus::AddressingMode::Undefined
#define AM_ACCUMULATOR MOS6502::DataBus::AddressingMode::Accumulator
#define AM_ABSOLUTE MOS6502::DataBus::AddressingMode::Absolute
#define AM_ABSOLUTE_X_INDEXED MOS6502::DataBus::AddressingMode::AbsoluteXIndexed
#define AM_ABSOLUTE_Y_INDEXED MOS6502::DataBus::AddressingMode::AbsoluteYIndexed
#define AM_IMMEDIATE MOS6502::DataBus::AddressingMode::Immediate
#define AM_IMPLIED MOS6502::DataBus::AddressingMode::Implied
#define AM_INDIRECT MOS6502::DataBus::AddressingMode::Indirect
#define AM_X_INDEXED_INDIRECT MOS6502::DataBus::AddressingMode::XIndexedIndirect
#define AM_INDIRECT_Y_INDEXED MOS6502::DataBus::AddressingMode::IndirectYIndexed
#define AM_RELATIVE MOS6502::DataBus::AddressingMode::Relative
#define AM_ZEROPAGE MOS6502::DataBus::AddressingMode::Zeropage
#define AM_ZEROPAGE_X_INDEXED MOS6502::DataBus::AddressingMode::ZeropageXIndexed
#define AM_ZEROPAGE_Y_INDEXED MOS6502::DataBus::AddressingMode::ZeropageYIndexed


//Read operation, aware of databus addressing mode 
#define DB_READ_BYTE()										\
	if(m_DataBus.AddressingMode != AM_ACCUMULATOR)			\
		data_read();										\
	else													\
		m_DataBus.Data = m_Registers.AC;

//Write operation, aware of databus addressing mode 
#define DB_WRITE_BYTE()										\
	if(m_DataBus.AddressingMode != AM_ACCUMULATOR)			\
		data_write();										\
	else													\
		m_Registers.AC = m_DataBus.Data;

#pragma endregion

#pragma region -- Execution Helpers --

//-----------------------------------------------------------
#define DEBUG_ERROR() IsPause = true; IsError = true;
//#define DEBUG_PAUSE() m_Pipeline.Halt = true;
//-----------------------------------------------------------
//Prepare to read next byte
#define EH_SELECT_BYTE(OFFSET)								\
			m_DataBus.Address = (m_Registers.PC++) +		\
								(uint16_t)OFFSET;
//Read next byte
#define EH_FETCH_BYTE(OFFSET)								\
			EH_FETCH_BYTE_INDIRECT(m_Registers.PC,0)	    \
			m_DataBus.Address = m_DataBus.Data + OFFSET;	\
			m_Registers.PC++;

//Read byte from address
#define EH_FETCH_BYTE_INDIRECT(ADDRESS,OFFSET)				\
			m_DataBus.Address = (ADDRESS) +					\
								(uint16_t)OFFSET;			\
			data_read();									\
			m_DataBus.Address = m_DataBus.Data;				

//Read next word
#define EH_FETCH_WORD(OFFSET)								\
			EH_FETCH_WORD_INDIRECT(m_Registers.PC,OFFSET)	\
			m_Registers.PC+=2;

//Read word from address
#define EH_FETCH_WORD_INDIRECT(ADDRESS,OFFSET)				\
			m_DataBus.Address = ADDRESS;					\
			data_read();									\
			m_DataBus.Intermediate = m_DataBus.Data;		\
			m_DataBus.Address = ADDRESS+1;					\
			data_read();									\
			m_DataBus.Intermediate |= m_DataBus.Data << 8;	\
			m_DataBus.Address = m_DataBus.Intermediate +	\
								(uint16_t)OFFSET;
//-----------------------------------------------------------

//Implied mode
#define EH_MODE_IMP(_execute_func)							\
		m_DataBus.AddressingMode = AM_IMPLIED;				\
		_execute_func;

//Accumulator mode
#define EH_MODE_ACC(_execute_func)							\
		m_DataBus.AddressingMode = AM_ACCUMULATOR;			\
		_execute_func;

//Immidiate mode
#define EH_MODE_IMM(_execute_func)							\
		m_DataBus.AddressingMode = AM_IMMEDIATE;			\
		EH_SELECT_BYTE(0)									\
		_execute_func;

//Relative mode
#define EH_MODE_REL(_execute_func)							\
		m_DataBus.AddressingMode = AM_RELATIVE;				\
		EH_SELECT_BYTE(0)									\
		_execute_func;

//ZeroPage mode
#define EH_MODE_ZPG(_execute_func)							\
		m_DataBus.AddressingMode = AM_ZEROPAGE;				\
		EH_FETCH_BYTE(0)									\
		_execute_func;

#define EH_MODE_ZPX(_execute_func)							\
		m_DataBus.AddressingMode = AM_ZEROPAGE_X_INDEXED;	\
		EH_FETCH_BYTE(m_Registers.XR)						\
		m_DataBus.Address = m_DataBus.Address & 0x00FF;		\
		_execute_func;
	
#define EH_MODE_ZPY(_execute_func)							\
		m_DataBus.AddressingMode = AM_ZEROPAGE_Y_INDEXED;	\
		EH_FETCH_BYTE(m_Registers.YR)						\
		m_DataBus.Address = m_DataBus.Address & 0x00FF;		\
		_execute_func;

//Absolute mode
#define EH_MODE_ABS(_execute_func)							\
		m_DataBus.AddressingMode = AM_ABSOLUTE;				\
		EH_FETCH_WORD(0)									\
		_execute_func;

#define EH_MODE_ABX(_execute_func)							\
		m_DataBus.AddressingMode = AM_ABSOLUTE_X_INDEXED;	\
		EH_FETCH_WORD(m_Registers.XR)						\
		_execute_func;

#define EH_MODE_ABY(_execute_func)							\
		m_DataBus.AddressingMode = AM_ABSOLUTE_Y_INDEXED;	\
		EH_FETCH_WORD(m_Registers.YR)						\
		_execute_func;

//Indirect mode
#define EH_MODE_IND(_execute_func)							\
		m_DataBus.AddressingMode = AM_INDIRECT;				\
		EH_FETCH_WORD(0)									\
		m_DataBus.Address = m_DataBus.Intermediate;			\
		EH_FETCH_WORD_INDIRECT(m_DataBus.Address,0)			\
		_execute_func;

#define EH_MODE_XIN(_execute_func)							\
		m_DataBus.AddressingMode = AM_X_INDEXED_INDIRECT;	\
		EH_FETCH_BYTE(m_Registers.XR)						\
		m_DataBus.Address = m_DataBus.Address & 0x00FF;		\
		EH_FETCH_WORD_INDIRECT(m_DataBus.Address,0)			\
		_execute_func;			

#define EH_MODE_INY(_execute_func)								\
		m_DataBus.AddressingMode = AM_INDIRECT_Y_INDEXED;		\
		EH_FETCH_BYTE(0)										\
		m_DataBus.Address = m_DataBus.Address & 0x00FF;		\
		EH_FETCH_WORD_INDIRECT(m_DataBus.Address,m_Registers.YR)\
		_execute_func;
#pragma endregion


#pragma region -- Code Helpers --

#pragma endregion