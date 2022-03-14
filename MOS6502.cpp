#include "MOS6502.h"
#include <ctype.h>

MOS6502::MOS6502(Memory* ram, uint16_t stack_offset)
{
	m_MemoryPtr = ram;
	m_StackOffset = stack_offset;
	Disasm = false;
	reset();
}

MOS6502::~MOS6502()
{
}

void MOS6502::reset()
{
	m_Registers.PC = m_MemoryPtr->dma_read_dword(0xFFFC);
	m_Registers.AC = 0;
	m_Registers.XR = 0;
	m_Registers.YR = 0;
	m_Registers.SR = 0x20;
	m_Registers.SP = 0xFF;

	m_Pipeline.Break = false;
	m_Pipeline.InstructionCounter = 0;
	m_Pipeline.GlobalClockCycle = 0;
	m_Pipeline.InstructionCycle = 0;
	m_Pipeline.LastInstructionAddress = 0;
	m_Pipeline.LastInstructionOpCode = 0;
}

bool MOS6502::clock()
{
	return (IsPause == false) ? execute() : false;
}

bool MOS6502::execute()
{
#ifdef CPU_PRINT_DISASSEBLE 
	disassemble(nullptr,m_Registers.PC, true, false);
#endif

	m_Pipeline.GlobalClockCycle++;
	//if (m_Pipeline.InstructionCycle == 0)
	{
		//Global counters
		m_Pipeline.InstructionCounter++;
		m_Pipeline.InstructionCycle = 0;

		//Fetch opc from memory
		m_DataBus.Address = m_Registers.PC++;
		data_read();

		//Store opc
		m_Pipeline.LastInstructionAddress	=	m_DataBus.Address;
		m_Pipeline.LastInstructionOpCode	=	m_DataBus.Data;
	}
	//else
	{
		switch (m_Pipeline.LastInstructionOpCode)
		{
		case 0x00: EH_MODE_IMP(execute_brk()); break; //BRK impl
		case 0x01: EH_MODE_XIN(execute_ora()); break; //ORA X,ind
		case 0x05: EH_MODE_ZPG(execute_ora()); break; //ORA zpg
		case 0x06: EH_MODE_ZPG(execute_asl()); break; //ASL zpg
		case 0x08: EH_MODE_IMP(execute_php()); break; //PHP impl
		case 0x09: EH_MODE_IMM(execute_ora()); break; //ORA #
		case 0x0A: EH_MODE_ACC(execute_asl()); break; //ASL A
		case 0x0D: EH_MODE_ABS(execute_ora()); break; //ORA abs
		case 0x0E: EH_MODE_ABS(execute_asl()); break; //ASL abs
		case 0x10: EH_MODE_REL(execute_bpl()); break; //BPL rel
		case 0x11: EH_MODE_INY(execute_ora()); break; //ORA ind,Y
		case 0x15: EH_MODE_ZPX(execute_ora()); break; //ORA zpg,X
		case 0x16: EH_MODE_ZPX(execute_asl()); break; //ASL zpg,X
		case 0x18: EH_MODE_IMP(execute_clc()); break; //CLC impl
		case 0x19: EH_MODE_ABY(execute_ora()); break; //ORA abs,Y
		case 0x1D: EH_MODE_ABX(execute_ora()); break; //ORA abs,X
		case 0x1E: EH_MODE_ABX(execute_asl()); break; //ASL abs,X
		case 0x20: EH_MODE_ABS(execute_jsr()); break; //JSR abs
		case 0x21: EH_MODE_XIN(execute_and()); break; //AND X,ind
		case 0x24: EH_MODE_ZPG(execute_bit()); break; //BIT zpg
		case 0x25: EH_MODE_ZPG(execute_and()); break; //AND zpg
		case 0x26: EH_MODE_ZPG(execute_rol()); break; //ROL zpg
		case 0x28: EH_MODE_IMP(execute_plp()); break; //PLP impl
		case 0x29: EH_MODE_IMM(execute_and()); break; //AND #
		case 0x2A: EH_MODE_ACC(execute_rol()); break; //ROL A
		case 0x2C: EH_MODE_ABS(execute_bit()); break; //BIT abs
		case 0x2D: EH_MODE_ABS(execute_and()); break; //AND abs
		case 0x2E: EH_MODE_ABS(execute_rol()); break; //ROL abs
		case 0x30: EH_MODE_REL(execute_bmi()); break; //BMI rel
		case 0x31: EH_MODE_INY(execute_and()); break; //AND ind,Y
		case 0x35: EH_MODE_ZPX(execute_and()); break; //AND zpg,X
		case 0x36: EH_MODE_ZPX(execute_rol()); break; //ROL zpg,X
		case 0x38: EH_MODE_IMP(execute_sec()); break; //SEC impl
		case 0x39: EH_MODE_ABY(execute_and()); break; //AND abs,Y
		case 0x3D: EH_MODE_ABX(execute_and()); break; //AND abs,X
		case 0x3E: EH_MODE_ABX(execute_rol()); break; //ROL abs,X
		case 0x40: EH_MODE_IMP(execute_rti()); break; //RTI impl
		case 0x41: EH_MODE_XIN(execute_eor()); break; //EOR X,ind
		case 0x45: EH_MODE_ZPG(execute_eor()); break; //EOR zpg
		case 0x46: EH_MODE_ZPG(execute_lsr()); break; //LSR zpg
		case 0x48: EH_MODE_IMP(execute_pha()); break; //PHA impl
		case 0x49: EH_MODE_IMM(execute_eor()); break; //EOR #
		case 0x4A: EH_MODE_ACC(execute_lsr()); break; //LSR A
		case 0x4C: EH_MODE_ABS(execute_jmp()); break; //JMP abs
		case 0x4D: EH_MODE_ABS(execute_eor()); break; //EOR abs
		case 0x4E: EH_MODE_ABS(execute_lsr()); break; //LSR abs
		case 0x50: EH_MODE_REL(execute_bvc()); break; //BVC rel
		case 0x51: EH_MODE_INY(execute_eor()); break; //EOR ind,Y
		case 0x55: EH_MODE_ZPX(execute_eor()); break; //EOR zpg,X
		case 0x56: EH_MODE_ZPX(execute_lsr()); break; //LSR zpg,X
		case 0x58: EH_MODE_IMP(execute_cli()); break; //CLI impl
		case 0x59: EH_MODE_ABY(execute_eor()); break; //EOR abs,Y
		case 0x5D: EH_MODE_ABX(execute_eor()); break; //EOR abs,X
		case 0x5E: EH_MODE_ABX(execute_lsr()); break; //LSR abs,X
		case 0x60: EH_MODE_IMP(execute_rts()); break; //RTS impl
		case 0x61: EH_MODE_XIN(execute_adc()); break; //ADC X,ind
		case 0x65: EH_MODE_ZPG(execute_adc()); break; //ADC zpg
		case 0x66: EH_MODE_ZPG(execute_ror()); break; //ROR zpg
		case 0x68: EH_MODE_IMP(execute_pla()); break; //PLA impl
		case 0x69: EH_MODE_IMM(execute_adc()); break; //ADC #
		case 0x6A: EH_MODE_ACC(execute_ror()); break; //ROR A
		case 0x6C: EH_MODE_IND(execute_jmp()); break; //JMP ind
		case 0x6D: EH_MODE_ABS(execute_adc()); break; //ADC abs
		case 0x6E: EH_MODE_ABS(execute_ror()); break; //ROR abs
		case 0x70: EH_MODE_REL(execute_bvs()); break; //BVS rel
		case 0x71: EH_MODE_INY(execute_adc()); break; //ADC ind,Y
		case 0x75: EH_MODE_ZPX(execute_adc()); break; //ADC zpg,X
		case 0x76: EH_MODE_ZPX(execute_ror()); break; //ROR zpg,X
		case 0x78: EH_MODE_IMP(execute_sei()); break; //SEI impl
		case 0x79: EH_MODE_ABY(execute_adc()); break; //ADC abs,Y
		case 0x7D: EH_MODE_ABX(execute_adc()); break; //ADC abs,X
		case 0x7E: EH_MODE_ABX(execute_ror()); break; //ROR abs,X
		case 0x81: EH_MODE_XIN(execute_sta()); break; //STA X,ind
		case 0x84: EH_MODE_ZPG(execute_sty()); break; //STY zpg
		case 0x85: EH_MODE_ZPG(execute_sta()); break; //STA zpg
		case 0x86: EH_MODE_ZPG(execute_stx()); break; //STX zpg
		case 0x88: EH_MODE_IMP(execute_dey()); break; //DEY impl
		case 0x8A: EH_MODE_IMP(execute_txa()); break; //TXA impl
		case 0x8C: EH_MODE_ABS(execute_sty()); break; //STY abs
		case 0x8D: EH_MODE_ABS(execute_sta()); break; //STA abs
		case 0x8E: EH_MODE_ABS(execute_stx()); break; //STX abs
		case 0x90: EH_MODE_REL(execute_bcc()); break; //BCC rel
		case 0x91: EH_MODE_INY(execute_sta()); break; //STA ind,Y
		case 0x94: EH_MODE_ZPX(execute_sty()); break; //STY zpg,X
		case 0x95: EH_MODE_ZPX(execute_sta()); break; //STA zpg,X
		case 0x96: EH_MODE_ZPY(execute_stx()); break; //STX zpg,Y
		case 0x98: EH_MODE_IMP(execute_tya()); break; //TYA impl
		case 0x99: EH_MODE_ABY(execute_sta()); break; //STA abs,Y
		case 0x9A: EH_MODE_IMP(execute_txs()); break; //TXS impl
		case 0x9D: EH_MODE_ABX(execute_sta()); break; //STA abs,X
		case 0xA0: EH_MODE_IMM(execute_ldy()); break; //LDY #
		case 0xA1: EH_MODE_XIN(execute_lda()); break; //LDA X,ind
		case 0xA2: EH_MODE_IMM(execute_ldx()); break; //LDX #
		case 0xA4: EH_MODE_ZPG(execute_ldy()); break; //LDY zpg
		case 0xA5: EH_MODE_ZPG(execute_lda()); break; //LDA zpg
		case 0xA6: EH_MODE_ZPG(execute_ldx()); break; //LDX zpg
		case 0xA8: EH_MODE_IMP(execute_tay()); break; //TAY impl
		case 0xA9: EH_MODE_IMM(execute_lda()); break; //LDA #
		case 0xAA: EH_MODE_IMP(execute_tax()); break; //TAX impl
		case 0xAC: EH_MODE_ABS(execute_ldy()); break; //LDY abs
		case 0xAD: EH_MODE_ABS(execute_lda()); break; //LDA abs
		case 0xAE: EH_MODE_ABS(execute_ldx()); break; //LDX abs
		case 0xB0: EH_MODE_REL(execute_bcs()); break; //BCS rel
		case 0xB1: EH_MODE_INY(execute_lda()); break; //LDA ind,Y
		case 0xB4: EH_MODE_ZPX(execute_ldy()); break; //LDY zpg,X
		case 0xB5: EH_MODE_ZPX(execute_lda()); break; //LDA zpg,X
		case 0xB6: EH_MODE_ZPY(execute_ldx()); break; //LDX zpg,Y
		case 0xB8: EH_MODE_IMP(execute_clv()); break; //CLV impl
		case 0xB9: EH_MODE_ABY(execute_lda()); break; //LDA abs,Y
		case 0xBA: EH_MODE_IMP(execute_tsx()); break; //TSX impl
		case 0xBC: EH_MODE_ABX(execute_ldy()); break; //LDY abs,X
		case 0xBD: EH_MODE_ABX(execute_lda()); break; //LDA abs,X
		case 0xBE: EH_MODE_ABY(execute_ldx()); break; //LDX abs,Y
		case 0xC0: EH_MODE_IMM(execute_cpy()); break; //CPY #
		case 0xC1: EH_MODE_XIN(execute_cmp()); break; //CMP X,ind
		case 0xC4: EH_MODE_ZPG(execute_cpy()); break; //CPY zpg
		case 0xC5: EH_MODE_ZPG(execute_cmp()); break; //CMP zpg
		case 0xC6: EH_MODE_ZPG(execute_dec()); break; //DEC zpg
		case 0xC8: EH_MODE_IMP(execute_iny()); break; //INY impl
		case 0xC9: EH_MODE_IMM(execute_cmp()); break; //CMP #
		case 0xCA: EH_MODE_IMP(execute_dex()); break; //DEX impl
		case 0xCC: EH_MODE_ABS(execute_cpy()); break; //CPY abs
		case 0xCD: EH_MODE_ABS(execute_cmp()); break; //CMP abs
		case 0xCE: EH_MODE_ABS(execute_dec()); break; //DEC abs
		case 0xD0: EH_MODE_REL(execute_bne()); break; //BNE rel
		case 0xD1: EH_MODE_INY(execute_cmp()); break; //CMP ind,Y
		case 0xD5: EH_MODE_ZPX(execute_cmp()); break; //CMP zpg,X
		case 0xD6: EH_MODE_ZPX(execute_dec()); break; //DEC zpg,X
		case 0xD8: EH_MODE_IMP(execute_cld()); break; //CLD impl
		case 0xD9: EH_MODE_ABY(execute_cmp()); break; //CMP abs,Y
		case 0xDD: EH_MODE_ABX(execute_cmp()); break; //CMP abs,X
		case 0xDE: EH_MODE_ABX(execute_dec()); break; //DEC abs,X
		case 0xE0: EH_MODE_IMM(execute_cpx()); break; //CPX #
		case 0xE1: EH_MODE_XIN(execute_sbc()); break; //SBC X,ind
		case 0xE4: EH_MODE_ZPG(execute_cpx()); break; //CPX zpg
		case 0xE5: EH_MODE_ZPG(execute_sbc()); break; //SBC zpg
		case 0xE6: EH_MODE_ZPG(execute_inc()); break; //INC zpg
		case 0xE8: EH_MODE_IMP(execute_inx()); break; //INX impl
		case 0xE9: EH_MODE_IMM(execute_sbc()); break; //SBC #
		case 0xEA: EH_MODE_IMP(execute_nop()); break; //NOP impl
		case 0xEC: EH_MODE_ABS(execute_cpx()); break; //CPX abs
		case 0xED: EH_MODE_ABS(execute_sbc()); break; //SBC abs
		case 0xEE: EH_MODE_ABS(execute_inc()); break; //INC abs
		case 0xF0: EH_MODE_REL(execute_beq()); break; //BEQ rel
		case 0xF1: EH_MODE_INY(execute_sbc()); break; //SBC ind,Y
		case 0xF5: EH_MODE_ZPX(execute_sbc()); break; //SBC zpg,X
		case 0xF6: EH_MODE_ZPX(execute_inc()); break; //INC zpg,X
		case 0xF8: EH_MODE_IMP(execute_sed()); break; //SED impl
		case 0xF9: EH_MODE_ABY(execute_sbc()); break; //SBC abs,Y
		case 0xFD: EH_MODE_ABX(execute_sbc()); break; //SBC abs,X
		case 0xFE: EH_MODE_ABX(execute_inc()); break; //INC abs,X
		default:  execute_xxx();	break;
		}
	}

#ifdef CPU_STOP_ON_HANGUP_GENERAL
	if (IsPause && IsError)
	{
		IsError = false;
		printf("\n\n\t\t**************************\n\t\t*    ERROR : CPU FCKD    *\n\t\t************************** \n\n");

		printf("INSTRUCTION : \n\t");
		disassemble(nullptr,m_Pipeline.LastInstructionAddress, false, true);
		printf("\n");
		printf("REGISTERS : \n\tPC: %.4x  AC: %.2x  XR: %.2x  YR: %.2x  SP: %.2x  SR: %.2x\n\n",
			m_Registers.PC,
			m_Registers.AC,
			m_Registers.XR,
			m_Registers.YR,
			m_Registers.SP,
			m_Registers.SR
		);
		printf("PIPELINE : \n");
		printf("\tCOUNTER : %d\n", m_Pipeline.InstructionCounter);
		printf("\tCYCLES : %d\n", m_Pipeline.GlobalClockCycle);
		printf("\tPIPILINE_STEP : %d\n", m_Pipeline.InstructionCycle);
		printf("\n");
		printf("STACK : \n");
		for (int lines = 0; lines < 16; lines++)
		{
			printf("\t:%.4x\t", m_StackOffset + lines * 16);
			for (int columns = 0; columns < 16; columns++)
			{
				if ((m_StackOffset + (lines * 16) + columns) <= 0xFFFF)
					printf("%.2x ", m_MemoryPtr->data()[m_StackOffset + (lines * 16) + columns]);
				else
					printf("?? ");
			}
			printf("\t");
			for (int columns = 0; columns < 16; columns++)
			{
				if ((m_StackOffset + (lines * 16) + columns) > 0xFFFF)
					printf("_");
				else if (isprint(m_MemoryPtr->data()[(m_StackOffset + (lines * 16) + columns)]))
					printf("%c", m_MemoryPtr->data()[(m_StackOffset + (lines * 16) + columns)]);
				else
					printf(".");
			}
			printf("\n");
		}
		printf("\n");
		printf("MEMORY : \n");

		uint16_t first_address = (m_Registers.PC > 0xFF) ? m_Registers.PC - 0xFF : 0x0000;
		for (int lines = 0; lines < 32; lines++)
		{
			if ((first_address + (lines * 16)) > 0xFFFF)
				printf("\t:____\t");
			else
				printf("\t:%.4x\t", first_address + lines * 16);
			for (int columns = 0; columns < 16; columns++)
			{
				if ((first_address + (lines * 16) + columns) > 0xFFFF)
					printf("__ ");
				else
					printf("%.2x ", m_MemoryPtr->data()[(first_address + (lines * 16) + columns)]);
			}

			printf("\t");
			for (int columns = 0; columns < 16; columns++)
			{
				if ((first_address + (lines * 16) + columns) > 0xFFFF)
					printf("_");
				else if (isprint(m_MemoryPtr->data()[(first_address + (lines * 16) + columns)]))
					printf("%c", m_MemoryPtr->data()[(first_address + (lines * 16) + columns)]);
				else
					printf(".");
			}
			printf("\n");
		}
		printf("\n");
		return false;
	}
#endif

	return !m_Pipeline.Break;
}

void MOS6502::interrupt()
{
	if (m_Registers.getFlag(SR_INTERRUPTBIT) == false)
	{
		m_DataBus.Address = m_StackOffset + m_Registers.SP--;
		m_DataBus.Data = (m_Registers.PC & 0xFF00) >> 8;
		data_write();

		m_DataBus.Address = m_StackOffset + m_Registers.SP--;
		m_DataBus.Data = (m_Registers.PC & 0x00FF);
		data_write();

		m_DataBus.Address = m_StackOffset + m_Registers.SP--;
		m_DataBus.Data = (m_Registers.SR);
		data_write();

		m_Registers.setFlag(SR_INTERRUPTBIT, true);
		m_Registers.setFlag(SR_BREAKBIT, false);

		
		m_Registers.PC = m_MemoryPtr->dma_read_dword(0xFFFE);
	}
}

void MOS6502::disassemble(char* out_buffer, uint16_t address, bool add_registers,bool force)
{
	if (Disasm || force )
	{
		char buffer[64] = { 0 };
		switch (m_MemoryPtr->dma_read_word(address))
		{
		case 0x00: sprintf(buffer, ":%0.4X  %.2x            BRK ", address, m_MemoryPtr->dma_read_word(address)); break; //BRK impl
		case 0x01: sprintf(buffer, ":%0.4X  %.2x %.2x         ORA ($%0.2X,X)", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //ORA X,ind
		case 0x05: sprintf(buffer, ":%0.4X  %.2x %.2x         ORA $%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //ORA zpg
		case 0x06: sprintf(buffer, ":%0.4X  %.2x %.2x         ASL $%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //ASL zpg
		case 0x08: sprintf(buffer, ":%0.4X  %.2x            PHP ", address, m_MemoryPtr->dma_read_word(address)); break; //PHP impl
		case 0x09: sprintf(buffer, ":%0.4X  %.2x %.2x         ORA #$%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //ORA #
		case 0x0A: sprintf(buffer, ":%0.4X  %.2x            ASL A", address, m_MemoryPtr->dma_read_word(address)); break; //ASL A
		case 0x0D: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      ORA $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //ORA abs
		case 0x0E: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      ASL $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //ASL abs
		case 0x10: sprintf(buffer, ":%0.4X  %.2x %.2x         BPL $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), address + (int8_t)m_MemoryPtr->dma_read_word(address + 1) + 2); break; //BPL rel
		case 0x11: sprintf(buffer, ":%0.4X  %.2x %.2x         ORA ($%0.2X),Y", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //ORA ind,Y
		case 0x15: sprintf(buffer, ":%0.4X  %.2x %.2x         ORA $%0.2X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //ORA zpg,X
		case 0x16: sprintf(buffer, ":%0.4X  %.2x %.2x         ASL $%0.2X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //ASL zpg,X
		case 0x18: sprintf(buffer, ":%0.4X  %.2x            CLC ", address, m_MemoryPtr->dma_read_word(address)); break; //CLC impl
		case 0x19: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      ORA $%0.4X,Y", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //ORA abs,Y
		case 0x1D: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      ORA $%0.4X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //ORA abs,X
		case 0x1E: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      ASL $%0.4X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //ASL abs,X
		case 0x20: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      JSR $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //JSR abs
		case 0x21: sprintf(buffer, ":%0.4X  %.2x %.2x         AND ($%0.2X,X)", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //AND X,ind
		case 0x24: sprintf(buffer, ":%0.4X  %.2x %.2x         BIT $%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //BIT zpg
		case 0x25: sprintf(buffer, ":%0.4X  %.2x %.2x         AND $%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //AND zpg
		case 0x26: sprintf(buffer, ":%0.4X  %.2x %.2x         ROL $%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //ROL zpg
		case 0x28: sprintf(buffer, ":%0.4X  %.2x            PLP ", address, m_MemoryPtr->dma_read_word(address)); break; //PLP impl
		case 0x29: sprintf(buffer, ":%0.4X  %.2x %.2x         AND #$%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //AND #
		case 0x2A: sprintf(buffer, ":%0.4X  %.2x            ROL A", address, m_MemoryPtr->dma_read_word(address)); break; //ROL A
		case 0x2C: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      BIT $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //BIT abs
		case 0x2D: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      AND $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //AND abs
		case 0x2E: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      ROL $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //ROL abs
		case 0x30: sprintf(buffer, ":%0.4X  %.2x %.2x         BMI $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), address + (int8_t)m_MemoryPtr->dma_read_word(address + 1) + 2); break; //BMI rel
		case 0x31: sprintf(buffer, ":%0.4X  %.2x %.2x         AND ($%0.2X),Y", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //AND ind,Y
		case 0x35: sprintf(buffer, ":%0.4X  %.2x %.2x         AND $%0.2X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //AND zpg,X
		case 0x36: sprintf(buffer, ":%0.4X  %.2x %.2x         ROL $%0.2X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //ROL zpg,X
		case 0x38: sprintf(buffer, ":%0.4X  %.2x            SEC ", address, m_MemoryPtr->dma_read_word(address)); break; //SEC impl
		case 0x39: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      AND $%0.4X,Y", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //AND abs,Y
		case 0x3D: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      AND $%0.4X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //AND abs,X
		case 0x3E: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      ROL $%0.4X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //ROL abs,X
		case 0x40: sprintf(buffer, ":%0.4X  %.2x            RTI ", address, m_MemoryPtr->dma_read_word(address)); break; //RTI impl
		case 0x41: sprintf(buffer, ":%0.4X  %.2x %.2x         EOR ($%0.2X,X)", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //EOR X,ind
		case 0x45: sprintf(buffer, ":%0.4X  %.2x %.2x         EOR $%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //EOR zpg
		case 0x46: sprintf(buffer, ":%0.4X  %.2x %.2x         LSR $%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //LSR zpg
		case 0x48: sprintf(buffer, ":%0.4X  %.2x            PHA ", address, m_MemoryPtr->dma_read_word(address)); break; //PHA impl
		case 0x49: sprintf(buffer, ":%0.4X  %.2x %.2x         EOR #$%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //EOR #
		case 0x4A: sprintf(buffer, ":%0.4X  %.2x            LSR A", address, m_MemoryPtr->dma_read_word(address)); break; //LSR A
		case 0x4C: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      JMP $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //JMP abs
		case 0x4D: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      EOR $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //EOR abs
		case 0x4E: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      LSR $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //LSR abs
		case 0x50: sprintf(buffer, ":%0.4X  %.2x %.2x         BVC $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), address + (int8_t)m_MemoryPtr->dma_read_word(address + 1) + 2); break; //BVC rel
		case 0x51: sprintf(buffer, ":%0.4X  %.2x %.2x         EOR ($%0.2X),Y", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //EOR ind,Y
		case 0x55: sprintf(buffer, ":%0.4X  %.2x %.2x         EOR $%0.2X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //EOR zpg,X
		case 0x56: sprintf(buffer, ":%0.4X  %.2x %.2x         LSR $%0.2X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //LSR zpg,X
		case 0x58: sprintf(buffer, ":%0.4X  %.2x            CLI ", address, m_MemoryPtr->dma_read_word(address)); break; //CLI impl
		case 0x59: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      EOR $%0.4X,Y", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //EOR abs,Y
		case 0x5D: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      EOR $%0.4X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //EOR abs,X
		case 0x5E: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      LSR $%0.4X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //LSR abs,X
		case 0x60: sprintf(buffer, ":%0.4X  %.2x            RTS ", address, m_MemoryPtr->dma_read_word(address)); break; //RTS impl
		case 0x61: sprintf(buffer, ":%0.4X  %.2x %.2x         ADC ($%0.2X,X)", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //ADC X,ind
		case 0x65: sprintf(buffer, ":%0.4X  %.2x %.2x         ADC $%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //ADC zpg
		case 0x66: sprintf(buffer, ":%0.4X  %.2x %.2x         ROR $%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //ROR zpg
		case 0x68: sprintf(buffer, ":%0.4X  %.2x            PLA ", address, m_MemoryPtr->dma_read_word(address)); break; //PLA impl
		case 0x69: sprintf(buffer, ":%0.4X  %.2x %.2x         ADC #$%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //ADC #
		case 0x6A: sprintf(buffer, ":%0.4X  %.2x            ROR A", address, m_MemoryPtr->dma_read_word(address)); break; //ROR A
		case 0x6C: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      JMP ($%0.4X)", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //JMP ind
		case 0x6D: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      ADC $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //ADC abs
		case 0x6E: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      ROR $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //ROR abs
		case 0x70: sprintf(buffer, ":%0.4X  %.2x %.2x         BVS $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), address + (int8_t)m_MemoryPtr->dma_read_word(address + 1) + 2); break; //BVS rel
		case 0x71: sprintf(buffer, ":%0.4X  %.2x %.2x         ADC ($%0.2X),Y", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //ADC ind,Y
		case 0x75: sprintf(buffer, ":%0.4X  %.2x %.2x         ADC $%0.2X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //ADC zpg,X
		case 0x76: sprintf(buffer, ":%0.4X  %.2x %.2x         ROR $%0.2X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //ROR zpg,X
		case 0x78: sprintf(buffer, ":%0.4X  %.2x            SEI ", address, m_MemoryPtr->dma_read_word(address)); break; //SEI impl
		case 0x79: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      ADC $%0.4X,Y", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //ADC abs,Y
		case 0x7D: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      ADC $%0.4X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //ADC abs,X
		case 0x7E: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      ROR $%0.4X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //ROR abs,X
		case 0x81: sprintf(buffer, ":%0.4X  %.2x %.2x         STA ($%0.2X,X)", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //STA X,ind
		case 0x84: sprintf(buffer, ":%0.4X  %.2x %.2x         STY $%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //STY zpg
		case 0x85: sprintf(buffer, ":%0.4X  %.2x %.2x         STA $%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //STA zpg
		case 0x86: sprintf(buffer, ":%0.4X  %.2x %.2x         STX $%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //STX zpg
		case 0x88: sprintf(buffer, ":%0.4X  %.2x            DEY ", address, m_MemoryPtr->dma_read_word(address)); break; //DEY impl
		case 0x8A: sprintf(buffer, ":%0.4X  %.2x            TXA ", address, m_MemoryPtr->dma_read_word(address)); break; //TXA impl
		case 0x8C: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      STY $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //STY abs
		case 0x8D: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      STA $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //STA abs
		case 0x8E: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      STX $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //STX abs
		case 0x90: sprintf(buffer, ":%0.4X  %.2x %.2x         BCC $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), address + (int8_t)m_MemoryPtr->dma_read_word(address + 1) + 2); break; //BCC rel
		case 0x91: sprintf(buffer, ":%0.4X  %.2x %.2x         STA ($%0.2X),Y", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //STA ind,Y
		case 0x94: sprintf(buffer, ":%0.4X  %.2x %.2x         STY $%0.2X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //STY zpg,X
		case 0x95: sprintf(buffer, ":%0.4X  %.2x %.2x         STA $%0.2X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //STA zpg,X
		case 0x96: sprintf(buffer, ":%0.4X  %.2x %.2x         STX $%0.2X,Y", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //STX zpg,Y
		case 0x98: sprintf(buffer, ":%0.4X  %.2x            TYA ", address, m_MemoryPtr->dma_read_word(address)); break; //TYA impl
		case 0x99: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      STA $%0.4X,Y", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //STA abs,Y
		case 0x9A: sprintf(buffer, ":%0.4X  %.2x            TXS ", address, m_MemoryPtr->dma_read_word(address)); break; //TXS impl
		case 0x9D: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      STA $%0.4X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //STA abs,X
		case 0xA0: sprintf(buffer, ":%0.4X  %.2x %.2x         LDY #$%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //LDY #
		case 0xA1: sprintf(buffer, ":%0.4X  %.2x %.2x         LDA ($%0.2X,X)", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //LDA X,ind
		case 0xA2: sprintf(buffer, ":%0.4X  %.2x %.2x         LDX #$%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //LDX #
		case 0xA4: sprintf(buffer, ":%0.4X  %.2x %.2x         LDY $%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //LDY zpg
		case 0xA5: sprintf(buffer, ":%0.4X  %.2x %.2x         LDA $%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //LDA zpg
		case 0xA6: sprintf(buffer, ":%0.4X  %.2x %.2x         LDX $%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //LDX zpg
		case 0xA8: sprintf(buffer, ":%0.4X  %.2x            TAY ", address, m_MemoryPtr->dma_read_word(address)); break; //TAY impl
		case 0xA9: sprintf(buffer, ":%0.4X  %.2x %.2x         LDA #$%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //LDA #
		case 0xAA: sprintf(buffer, ":%0.4X  %.2x            TAX ", address, m_MemoryPtr->dma_read_word(address)); break; //TAX impl
		case 0xAC: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      LDY $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //LDY abs
		case 0xAD: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      LDA $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //LDA abs
		case 0xAE: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      LDX $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //LDX abs
		case 0xB0: sprintf(buffer, ":%0.4X  %.2x %.2x         BCS $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), address + (int8_t)m_MemoryPtr->dma_read_word(address + 1) + 2); break; //BCS rel
		case 0xB1: sprintf(buffer, ":%0.4X  %.2x %.2x         LDA ($%0.2X),Y", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //LDA ind,Y
		case 0xB4: sprintf(buffer, ":%0.4X  %.2x %.2x         LDY $%0.2X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //LDY zpg,X
		case 0xB5: sprintf(buffer, ":%0.4X  %.2x %.2x         LDA $%0.2X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //LDA zpg,X
		case 0xB6: sprintf(buffer, ":%0.4X  %.2x %.2x         LDX $%0.2X,Y", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //LDX zpg,Y
		case 0xB8: sprintf(buffer, ":%0.4X  %.2x            CLV ", address, m_MemoryPtr->dma_read_word(address)); break; //CLV impl
		case 0xB9: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      LDA $%0.4X,Y", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //LDA abs,Y
		case 0xBA: sprintf(buffer, ":%0.4X  %.2x            TSX ", address, m_MemoryPtr->dma_read_word(address)); break; //TSX impl
		case 0xBC: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      LDY $%0.4X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //LDY abs,X
		case 0xBD: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      LDA $%0.4X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //LDA abs,X
		case 0xBE: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      LDX $%0.4X,Y", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //LDX abs,Y
		case 0xC0: sprintf(buffer, ":%0.4X  %.2x %.2x         CPY #$%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //CPY #
		case 0xC1: sprintf(buffer, ":%0.4X  %.2x %.2x         CMP ($%0.2X,X)", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //CMP X,ind
		case 0xC4: sprintf(buffer, ":%0.4X  %.2x %.2x         CPY $%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //CPY zpg
		case 0xC5: sprintf(buffer, ":%0.4X  %.2x %.2x         CMP $%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //CMP zpg
		case 0xC6: sprintf(buffer, ":%0.4X  %.2x %.2x         DEC $%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //DEC zpg
		case 0xC8: sprintf(buffer, ":%0.4X  %.2x            INY ", address, m_MemoryPtr->dma_read_word(address)); break; //INY impl
		case 0xC9: sprintf(buffer, ":%0.4X  %.2x %.2x         CMP #$%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //CMP #
		case 0xCA: sprintf(buffer, ":%0.4X  %.2x            DEX ", address, m_MemoryPtr->dma_read_word(address)); break; //DEX impl
		case 0xCC: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      CPY $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //CPY abs
		case 0xCD: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      CMP $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //CMP abs
		case 0xCE: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      DEC $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //DEC abs
		case 0xD0: sprintf(buffer, ":%0.4X  %.2x %.2x         BNE $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), address + (int8_t)m_MemoryPtr->dma_read_word(address + 1) + 2); break; //BNE rel
		case 0xD1: sprintf(buffer, ":%0.4X  %.2x %.2x         CMP ($%0.2X),Y", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //CMP ind,Y
		case 0xD5: sprintf(buffer, ":%0.4X  %.2x %.2x         CMP $%0.2X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //CMP zpg,X
		case 0xD6: sprintf(buffer, ":%0.4X  %.2x %.2x         DEC $%0.2X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //DEC zpg,X
		case 0xD8: sprintf(buffer, ":%0.4X  %.2x            CLD ", address, m_MemoryPtr->dma_read_word(address)); break; //CLD impl
		case 0xD9: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      CMP $%0.4X,Y", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //CMP abs,Y
		case 0xDD: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      CMP $%0.4X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //CMP abs,X
		case 0xDE: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      DEC $%0.4X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //DEC abs,X
		case 0xE0: sprintf(buffer, ":%0.4X  %.2x %.2x         CPX #$%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //CPX #
		case 0xE1: sprintf(buffer, ":%0.4X  %.2x %.2x         SBC ($%0.2X,X)", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //SBC X,ind
		case 0xE4: sprintf(buffer, ":%0.4X  %.2x %.2x         CPX $%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //CPX zpg
		case 0xE5: sprintf(buffer, ":%0.4X  %.2x %.2x         SBC $%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //SBC zpg
		case 0xE6: sprintf(buffer, ":%0.4X  %.2x %.2x         INC $%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //INC zpg
		case 0xE8: sprintf(buffer, ":%0.4X  %.2x            INX ", address, m_MemoryPtr->dma_read_word(address)); break; //INX impl
		case 0xE9: sprintf(buffer, ":%0.4X  %.2x %.2x         SBC #$%0.2X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //SBC #
		case 0xEA: sprintf(buffer, ":%0.4X  %.2x            NOP ", address, m_MemoryPtr->dma_read_word(address)); break; //NOP impl
		case 0xEC: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      CPX $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //CPX abs
		case 0xED: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      SBC $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //SBC abs
		case 0xEE: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      INC $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //INC abs
		case 0xF0: sprintf(buffer, ":%0.4X  %.2x %.2x         BEQ $%0.4X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), address + (int8_t)m_MemoryPtr->dma_read_word(address + 1) + 2); break; //BEQ rel
		case 0xF1: sprintf(buffer, ":%0.4X  %.2x %.2x         SBC ($%0.2X),Y", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //SBC ind,Y
		case 0xF5: sprintf(buffer, ":%0.4X  %.2x %.2x         SBC $%0.2X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //SBC zpg,X
		case 0xF6: sprintf(buffer, ":%0.4X  %.2x %.2x         INC $%0.2X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 1)); break; //INC zpg,X
		case 0xF8: sprintf(buffer, ":%0.4X  %.2x            SED ", address, m_MemoryPtr->dma_read_word(address)); break; //SED impl
		case 0xF9: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      SBC $%0.4X,Y", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //SBC abs,Y
		case 0xFD: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      SBC $%0.4X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //SBC abs,X
		case 0xFE: sprintf(buffer, ":%0.4X  %.2x %.2x %.2x      INC $%0.4X,X", address, m_MemoryPtr->dma_read_word(address), m_MemoryPtr->dma_read_word(address + 1), m_MemoryPtr->dma_read_word(address + 2), m_MemoryPtr->dma_read_dword(address + 1)); break; //INC abs,X

		default:
			sprintf(buffer, ":%0.4x  %.2x            ???", address, m_MemoryPtr->dma_read_word(address)); break; //unknown
			break;
		}
		if (add_registers)
		{
			printf("%-32s\t\t;  PC: %.4x  AC : %.2x  XR : %.2x  YR : %.2x  SP : %.2x  SR : %.2x CYCLE : %.4d\n",
				buffer,
				m_Registers.PC,
				m_Registers.AC,
				m_Registers.XR,
				m_Registers.YR,
				m_Registers.SP,
				m_Registers.SR,
				m_Pipeline.GlobalClockCycle
			);
		}
		else
		{
			if (out_buffer != nullptr)
			{
				sprintf(out_buffer,"%-32s\n", buffer);
			}
			else
			{
				printf("%-32s\n", buffer);
			}
		}
	}
}

//--------------------------------------------------------------------------------------------------------------------------

void MOS6502::data_read()
{
#ifdef CPU_PRINT_MEMORYACCESS
	if (Disasm)
		printf("data_read $%.4x [$%.2x]\n", m_DataBus.Address, m_MemoryPtr->dma_read_word(m_DataBus.Address));
#endif // CPU_PRINT_MEMORYACCESS
	m_DataBus.Data = m_MemoryPtr->read(m_DataBus.Address);
}

void MOS6502::data_write()
{
#ifdef CPU_PRINT_MEMORYACCESS
	if (Disasm)
	printf("data_write $%.4x [$%.2x]\n", m_DataBus.Address, m_DataBus.Data);
#endif // CPU_PRINT_MEMORYACCESS
	m_MemoryPtr->write(m_DataBus.Address, m_DataBus.Data);
}

//--------------------------------------------------------------------------------------------------------------------------

void MOS6502::execute_adc() // add with carry
{
	data_read();
	uint16_t adc_intermediate = (uint16_t)m_Registers.AC + (uint16_t)m_DataBus.Data + (m_Registers.getFlag(SR_CARRYBIT) ? 1 : 0);
	
	m_Registers.setFlag(SR_ZEROBIT, (adc_intermediate & 0xFF) == 0x0000);

	if (m_Registers.getFlag(SR_DECIMALBIT) == true)
	{
		if (((m_Registers.AC & 0xF) + (m_DataBus.Data & 0xF) + (m_Registers.getFlag(SR_CARRYBIT) ? 1 : 0)) > 9) adc_intermediate += 6;
		m_Registers.setFlag(SR_NEGATIVEBIT, (adc_intermediate & 0x80));
		m_Registers.setFlag(SR_OVERFLOWBIT, !((m_Registers.AC ^ m_DataBus.Data) & 0x80) && ((m_Registers.AC ^ adc_intermediate) & 0x80));
		if (adc_intermediate > 0x99) {
			adc_intermediate += 96;
		}
		m_Registers.setFlag(SR_CARRYBIT, (adc_intermediate > 0x99));
	}
	else
	{
		m_Registers.setFlag(SR_OVERFLOWBIT, !((m_Registers.AC ^ m_DataBus.Data) & 0x80) && ((m_Registers.AC ^ adc_intermediate) & 0x80));
		m_Registers.setFlag(SR_CARRYBIT, (adc_intermediate > 0xFF));
		m_Registers.setFlag(SR_NEGATIVEBIT, adc_intermediate & 0x80);
	}

	m_Registers.AC = (adc_intermediate & 0x00FF);
}
void MOS6502::execute_and() // and (with accumulator)
{
	data_read();
	m_Registers.AC &= m_DataBus.Data;
	m_Registers.setFlag(SR_ZEROBIT, m_Registers.AC == 0);
	m_Registers.setFlag(SR_NEGATIVEBIT, m_Registers.AC & 0x80);
}
void MOS6502::execute_asl() // arithmetic shift left
{
	DB_READ_BYTE();
	m_Registers.setFlag(SR_CARRYBIT, (m_DataBus.Data & 0x80));
	m_DataBus.Data = (m_DataBus.Data << 1);
	m_Registers.setFlag(SR_ZEROBIT, m_DataBus.Data == 0);
	m_Registers.setFlag(SR_NEGATIVEBIT, m_DataBus.Data & 0x80);
	DB_WRITE_BYTE();
}
void MOS6502::execute_bcc() // branch on carry clear
{
	data_read();
	if (m_Registers.getFlag(SR_CARRYBIT) == false)
	{
		m_Registers.PC += static_cast<int8_t>(m_DataBus.Data);
	}
	if (m_Registers.PC == m_Pipeline.LastInstructionAddress) DEBUG_ERROR();
}
void MOS6502::execute_bcs() // branch on carry set
{
	data_read();
	if (m_Registers.getFlag(SR_CARRYBIT) == true)
	{
		m_Registers.PC += static_cast<int8_t>(m_DataBus.Data);
	}
	if (m_Registers.PC == m_Pipeline.LastInstructionAddress) DEBUG_ERROR();
}
void MOS6502::execute_beq() // branch on equal (zero set)
{
	data_read();
	if (m_Registers.getFlag(SR_ZEROBIT) == true)
	{
		m_Registers.PC += static_cast<int8_t>(m_DataBus.Data);
	}
	if (m_Registers.PC == m_Pipeline.LastInstructionAddress) DEBUG_ERROR();
}
void MOS6502::execute_bit() // bit test
{
	data_read();
	uint8_t bit_intermediate = m_Registers.AC & m_DataBus.Data;

	m_Registers.SR = (m_Registers.SR & 0x3F) | (m_DataBus.Data & 0xC0);
	m_Registers.setFlag(SR_ZEROBIT, bit_intermediate == 0);
}
void MOS6502::execute_bmi() // branch on minus (negative set)
{
	data_read();
	if (m_Registers.getFlag(SR_NEGATIVEBIT) == true)
	{
		m_Registers.PC += static_cast<int8_t>(m_DataBus.Data);
	}
	if (m_Registers.PC == m_Pipeline.LastInstructionAddress) DEBUG_ERROR();
}
void MOS6502::execute_bne() // branch on not equal (zero clear)
{
	data_read();
	if (m_Registers.getFlag(SR_ZEROBIT) == false)
	{
		m_Registers.PC += static_cast<int8_t>(m_DataBus.Data);
	}
	if (m_Registers.PC == m_Pipeline.LastInstructionAddress) DEBUG_ERROR();
}
void MOS6502::execute_bpl() // branch on plus (negative clear)
{
	data_read();
	if (m_Registers.getFlag(SR_NEGATIVEBIT) == false)
	{
		m_Registers.PC += static_cast<int8_t>(m_DataBus.Data);
	}
	if (m_Registers.PC == m_Pipeline.LastInstructionAddress) DEBUG_ERROR();
}
void MOS6502::execute_brk() // break / interrupt
{
	m_DataBus.Address = m_StackOffset + m_Registers.SP--;
	m_DataBus.Data = (m_Registers.PC & 0xFF00) >> 8;
	data_write();

	m_DataBus.Address = m_StackOffset + m_Registers.SP--;
	m_DataBus.Data = (m_Registers.PC & 0x00FF) - 1;
	data_write();

	m_DataBus.Address = m_StackOffset + m_Registers.SP--;
	m_DataBus.Data = (m_Registers.SR | 0x10);
	data_write();

	m_Registers.setFlag(SR_INTERRUPTBIT, true);
	m_Registers.setFlag(SR_BREAKBIT, false);

	DEBUG_ERROR();
	m_Registers.PC = m_MemoryPtr->dma_read_dword(0xFFFE);
}
void MOS6502::execute_bvc() // branch on overflow clear
{
	data_read();
	if (m_Registers.getFlag(SR_OVERFLOWBIT) == false)
	{
		m_Registers.PC += static_cast<int8_t>(m_DataBus.Data);
	}
	if (m_Registers.PC == m_Pipeline.LastInstructionAddress) DEBUG_ERROR();
}
void MOS6502::execute_bvs() // branch on overflow set
{
	data_read();
	if (m_Registers.getFlag(SR_OVERFLOWBIT) == true)
	{
		m_Registers.PC += static_cast<int8_t>(m_DataBus.Data);
	}
	if (m_Registers.PC == m_Pipeline.LastInstructionAddress) DEBUG_ERROR();
}
void MOS6502::execute_clc() // clear carry
{
	m_Registers.setFlag(SR_CARRYBIT, false);
}
void MOS6502::execute_cld() // clear decimal
{
	m_Registers.setFlag(SR_DECIMALBIT, false);
}
void MOS6502::execute_cli() // clear interrupt disable
{
	m_Registers.setFlag(SR_INTERRUPTBIT, false);
}
void MOS6502::execute_clv() // clear overflow
{
	m_Registers.setFlag(SR_OVERFLOWBIT, false);
}
void MOS6502::execute_cmp() // compare (with accumulator)
{
	data_read();
	uint16_t cmp_intermediate = (uint16_t)m_Registers.AC - (uint16_t)m_DataBus.Data;

	m_Registers.setFlag(SR_CARRYBIT, m_Registers.AC >= m_DataBus.Data);
	m_Registers.setFlag(SR_ZEROBIT, (cmp_intermediate & 0xFF) == 0x0000);
	m_Registers.setFlag(SR_NEGATIVEBIT, cmp_intermediate & 0x80);
}
void MOS6502::execute_cpx() // compare with x
{
	data_read();
	uint16_t cmp_intermediate = (uint16_t)m_Registers.XR - (uint16_t)m_DataBus.Data;

	m_Registers.setFlag(SR_CARRYBIT, m_Registers.XR >= m_DataBus.Data);
	m_Registers.setFlag(SR_ZEROBIT, (cmp_intermediate & 0xFF) == 0x0000);
	m_Registers.setFlag(SR_NEGATIVEBIT, cmp_intermediate & 0x80);
}
void MOS6502::execute_cpy() // compare with y
{
	data_read();
	uint16_t cmp_intermediate = (uint16_t)m_Registers.YR - (uint16_t)m_DataBus.Data;

	m_Registers.setFlag(SR_CARRYBIT, m_Registers.YR >= m_DataBus.Data);
	m_Registers.setFlag(SR_ZEROBIT, (cmp_intermediate & 0xFF) == 0x0000);
	m_Registers.setFlag(SR_NEGATIVEBIT, cmp_intermediate & 0x80);
}
void MOS6502::execute_dec() // decrement
{
	data_read();
	m_DataBus.Data--;
	data_write();

	m_Registers.setFlag(SR_ZEROBIT, m_DataBus.Data == 0);
	m_Registers.setFlag(SR_NEGATIVEBIT, m_DataBus.Data & 0x80);
}
void MOS6502::execute_dex() // decrement x
{
	m_Registers.XR--;
	m_Registers.setFlag(SR_ZEROBIT, m_Registers.XR == 0);
	m_Registers.setFlag(SR_NEGATIVEBIT, m_Registers.XR & 0x80);
}
void MOS6502::execute_dey() // decrement y
{
	m_Registers.YR--;
	m_Registers.setFlag(SR_ZEROBIT, m_Registers.YR == 0);
	m_Registers.setFlag(SR_NEGATIVEBIT, m_Registers.YR & 0x80);
}
void MOS6502::execute_eor() // exclusive or (with accumulator)
{
	data_read();
	m_Registers.AC ^= m_DataBus.Data;

	m_Registers.setFlag(SR_ZEROBIT, m_Registers.AC == 0);
	m_Registers.setFlag(SR_NEGATIVEBIT, m_Registers.AC & 0x80);
}
void MOS6502::execute_inc() // increment
{
	data_read();
	m_DataBus.Data++;
	data_write();

	m_Registers.setFlag(SR_ZEROBIT, m_DataBus.Data == 0);
	m_Registers.setFlag(SR_NEGATIVEBIT, m_DataBus.Data & 0x80);
}
void MOS6502::execute_inx() // increment x
{
	m_Registers.XR++;
	m_Registers.setFlag(SR_ZEROBIT, m_Registers.XR == 0);
	m_Registers.setFlag(SR_NEGATIVEBIT, m_Registers.XR & 0x80);
}
void MOS6502::execute_iny() // increment y
{
	m_Registers.YR++;
	m_Registers.setFlag(SR_ZEROBIT, m_Registers.YR == 0);
	m_Registers.setFlag(SR_NEGATIVEBIT, m_Registers.YR & 0x80);
}
void MOS6502::execute_jmp() // jump
{
	if (m_DataBus.Address == m_Pipeline.LastInstructionAddress) DEBUG_ERROR();
	m_Registers.PC = m_DataBus.Address;
}
void MOS6502::execute_jsr() // jump subroutine
{
	uint16_t retPC = m_Pipeline.LastInstructionAddress + 2;

	m_DataBus.Address = m_StackOffset + m_Registers.SP--;	
	m_DataBus.Data = (retPC & 0xFF00) >> 8;
	data_write();

	m_DataBus.Address = m_StackOffset + m_Registers.SP--;
	m_DataBus.Data = (retPC & 0x00FF);
	data_write();

	m_Registers.PC = m_DataBus.Intermediate;
}
void MOS6502::execute_lda() // load accumulator
{
	data_read();
	m_Registers.AC = m_DataBus.Data;
	m_Registers.setFlag(SR_ZEROBIT, m_Registers.AC == 0);
	m_Registers.setFlag(SR_NEGATIVEBIT, m_Registers.AC & 0x80);
}
void MOS6502::execute_ldx() // load x
{
	data_read();
	m_Registers.XR = m_DataBus.Data;
	m_Registers.setFlag(SR_ZEROBIT, m_Registers.XR == 0);
	m_Registers.setFlag(SR_NEGATIVEBIT, m_Registers.XR & 0x80);
}
void MOS6502::execute_ldy() // load y
{
	data_read();
	m_Registers.YR = m_DataBus.Data;
	m_Registers.setFlag(SR_ZEROBIT, m_Registers.YR == 0);
	m_Registers.setFlag(SR_NEGATIVEBIT, m_Registers.YR & 0x80);
}
void MOS6502::execute_lsr() // logical shift right
{
	DB_READ_BYTE();
	m_Registers.setFlag(SR_CARRYBIT, (m_DataBus.Data & 0x01));
	m_DataBus.Data = (m_DataBus.Data >> 1);
	m_Registers.setFlag(SR_ZEROBIT, m_DataBus.Data == 0);
	m_Registers.setFlag(SR_NEGATIVEBIT, false);
	DB_WRITE_BYTE();
}
void MOS6502::execute_nop() // no operation
{
	
}
void MOS6502::execute_ora() // or with accumulator
{
	data_read();
	m_Registers.AC |= m_DataBus.Data;
	m_Registers.setFlag(SR_ZEROBIT, m_Registers.AC == 0);
	m_Registers.setFlag(SR_NEGATIVEBIT, m_Registers.AC & 0x80);
}
void MOS6502::execute_pha() // push accumulator
{
	m_DataBus.Address = m_StackOffset + m_Registers.SP--;
	m_DataBus.Data = (m_Registers.AC);
	data_write();
}
void MOS6502::execute_php() // push processor status (sr)
{
	m_DataBus.Address = m_StackOffset + m_Registers.SP--;
	m_DataBus.Data = (m_Registers.SR | 0x20);
	data_write();
}
void MOS6502::execute_pla() // pull accumulator
{
	m_DataBus.Address = m_StackOffset + (++m_Registers.SP);
	data_read();
	m_Registers.AC = m_DataBus.Data;

	m_Registers.setFlag(SR_ZEROBIT, m_Registers.AC == 0);
	m_Registers.setFlag(SR_NEGATIVEBIT, m_Registers.AC & 0x80);
}
void MOS6502::execute_plp() // pull processor status (sr)
{
	m_DataBus.Address = m_StackOffset + (++m_Registers.SP);
	data_read();
	m_Registers.SR = m_DataBus.Data;
}
void MOS6502::execute_rol() // rotate left
{
	DB_READ_BYTE();

	uint16_t rol_intermediate = m_DataBus.Data << 1 | (m_Registers.getFlag(SR_CARRYBIT) ? 0x01 : 0x00);
	m_DataBus.Data = (rol_intermediate & 0x00FF);
	
	m_Registers.setFlag(SR_CARRYBIT, rol_intermediate & 0xFF00);
	m_Registers.setFlag(SR_ZEROBIT, m_DataBus.Data == 0);
	m_Registers.setFlag(SR_NEGATIVEBIT, m_DataBus.Data & 0x80);

	DB_WRITE_BYTE();
}
void MOS6502::execute_ror() // rotate right
{
	DB_READ_BYTE();

	uint16_t ror_intermediate = m_DataBus.Data >> 1 | (m_Registers.getFlag(SR_CARRYBIT) ? 0x80 : 0x00);
	m_Registers.setFlag(SR_CARRYBIT, (m_DataBus.Data & 0x01));
	m_DataBus.Data = (ror_intermediate & 0x00FF);
	
	m_Registers.setFlag(SR_ZEROBIT, m_DataBus.Data == 0);
	m_Registers.setFlag(SR_NEGATIVEBIT, m_DataBus.Data & 0x80);

	DB_WRITE_BYTE();
}
void MOS6502::execute_rti() // return from interrupt
{
	//Pull status register
	m_DataBus.Address = m_StackOffset + (++m_Registers.SP);
	data_read();
	m_Registers.SR = m_DataBus.Data;

	//Pull PC register
	m_DataBus.Address = m_StackOffset + (++m_Registers.SP);
	data_read();
	m_DataBus.Intermediate = m_DataBus.Data;

	m_DataBus.Address = m_StackOffset + (++m_Registers.SP);
	data_read();
	m_DataBus.Intermediate |= m_DataBus.Data << 8;

	m_Registers.PC = m_DataBus.Intermediate;
}
void MOS6502::execute_rts() // return from subroutine
{
	m_DataBus.Address = m_StackOffset + (++m_Registers.SP);
	data_read();
	m_DataBus.Intermediate = m_DataBus.Data;

	m_DataBus.Address = m_StackOffset + (++m_Registers.SP);
	data_read();
	m_DataBus.Intermediate |= m_DataBus.Data << 8;

	m_Registers.PC = m_DataBus.Intermediate+1;
}
void MOS6502::execute_sbc() // subtract with carry
{
	data_read();
	
	uint16_t sbc_intermediate = ((uint16_t)m_Registers.AC - (uint16_t)m_DataBus.Data) - (m_Registers.getFlag(SR_CARRYBIT) ? 0 : 1);
	int16_t sbc_intermediate2 = ((int8_t)m_Registers.AC - (int8_t)m_DataBus.Data) - (m_Registers.getFlag(SR_CARRYBIT) ? 0 : 1);

	//m_Registers.setFlag(SR_OVERFLOWBIT, !(((uint16_t)m_Registers.AC ^ (uint16_t)m_DataBus.Data) & 0x80) && (((uint16_t)m_Registers.AC ^ sbc_intermediate) & 0x80));
	m_Registers.setFlag(SR_ZEROBIT, (sbc_intermediate & 0xFF) == 0x0000);
	m_Registers.setFlag(SR_NEGATIVEBIT, sbc_intermediate & 0x80);

	//fuck this overflow
	m_Registers.setFlag(SR_OVERFLOWBIT, (sbc_intermediate2 < -128 || sbc_intermediate2 > 127));
	/*printf("RES: %d %d\n", sbc_intermediate, (int16_t) sbc_intermediate);
	printf("orgi : u[%d %d] s[%d %d] h[%.2x %.2x]\n", m_Registers.AC, m_DataBus.Data, (int8_t)m_Registers.AC,(int8_t)m_DataBus.Data, m_Registers.AC, m_DataBus.Data);
	printf("OLD : %d\n", !(((uint16_t)m_Registers.AC ^ (uint16_t)m_DataBus.Data) & 0x80) && (((uint16_t)m_Registers.AC ^ sbc_intermediate) & 0x80));
	printf("NIV : %d [%d:$%.2x] a:%d x:%d\n", (sbc_intermediate2 < -128 || sbc_intermediate2 > 127), sbc_intermediate2, sbc_intermediate2, (int16_t)m_Registers.AC, (int16_t)((int8_t)m_DataBus.Data));*/
	//------------------

	if (m_Registers.getFlag(SR_DECIMALBIT) == true)
	{
		if (((m_Registers.AC & 0x0F) - (m_Registers.getFlag(SR_CARRYBIT) ? 0 : 1)) < (m_DataBus.Data & 0x0F)) sbc_intermediate -= 6;
		if (sbc_intermediate > 0x99)
		{
			sbc_intermediate -= 0x60;
		}
	}
	m_Registers.setFlag(SR_CARRYBIT, sbc_intermediate < 0x100);

	m_Registers.AC = (sbc_intermediate & 0x00FF);
}
void MOS6502::execute_sec() // set carry
{
	m_Registers.setFlag(SR_CARRYBIT, true);
}
void MOS6502::execute_sed() // set decimal
{
	m_Registers.setFlag(SR_DECIMALBIT, true);
}
void MOS6502::execute_sei() // set interrupt disable
{
	m_Registers.setFlag(SR_INTERRUPTBIT, true);
}
void MOS6502::execute_sta() // store accumulator
{
	m_DataBus.Data = m_Registers.AC;
	data_write();
}
void MOS6502::execute_stx() // store x
{
	m_DataBus.Data = m_Registers.XR;
	data_write();
}
void MOS6502::execute_sty() // store y
{
	m_DataBus.Data = m_Registers.YR;
	data_write();
}
void MOS6502::execute_tax() // transfer accumulator to x
{
	m_Registers.XR = m_Registers.AC;
	m_Registers.setFlag(SR_ZEROBIT, m_Registers.XR == 0);
	m_Registers.setFlag(SR_NEGATIVEBIT, m_Registers.XR & 0x80);
}
void MOS6502::execute_tay() // transfer accumulator to y
{
	m_Registers.YR = m_Registers.AC;
	m_Registers.setFlag(SR_ZEROBIT, m_Registers.YR == 0);
	m_Registers.setFlag(SR_NEGATIVEBIT, m_Registers.YR & 0x80);
}
void MOS6502::execute_tsx() // transfer stack pointer to x
{
	m_Registers.XR = m_Registers.SP;
	m_Registers.setFlag(SR_ZEROBIT, m_Registers.XR == 0);
	m_Registers.setFlag(SR_NEGATIVEBIT, m_Registers.XR & 0x80);
}
void MOS6502::execute_txa() // transfer x to accumulator
{
	m_Registers.AC = m_Registers.XR;
	m_Registers.setFlag(SR_ZEROBIT, m_Registers.AC == 0);
	m_Registers.setFlag(SR_NEGATIVEBIT, m_Registers.AC & 0x80);
}
void MOS6502::execute_txs() // transfer x to stack pointer
{
	m_Registers.SP = m_Registers.XR;
}
void MOS6502::execute_tya() // transfer y to accumulator 
{
	m_Registers.AC = m_Registers.YR;
	m_Registers.setFlag(SR_ZEROBIT, m_Registers.AC == 0);
	m_Registers.setFlag(SR_NEGATIVEBIT, m_Registers.AC & 0x80);
}

void MOS6502::execute_xxx()
{
	DEBUG_ERROR();
}
