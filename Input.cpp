#include "Input.h"

Input::Input(MOS6502* cpu, Memory* memory)
{
    m_Cpu = cpu;
    m_Memory = memory;
    for (int i = 0; i < 5; i++)
        m_Joystick[i] = 0;
}

Input::~Input()
{

}

bool Input::Initialize()
{
    //m_Memory->dma_write_word(0x9008, 0x80);
    //m_Memory->dma_write_word(0x9009, 0x80);

    return false;
}

bool Input::UpdateInput()
{
    if (IsPause)
        return false;

    UpdateTimers();
    UpdateKeyboard();
    
    return true;
}

void Input::setKeyPressed(uint32_t key, bool state)
{
    if (key == SDLK_LSHIFT && state) m_ShiftPressed = true; else if (key == SDLK_LSHIFT) m_ShiftPressed = false;
    if (key == SDLK_LCTRL && state) m_CBMPressed = true; else if (key == SDLK_LCTRL) m_CBMPressed = false;
    if (state)
        m_KeyPressed = ConvertKeyboardKeycode(key);
    else if (m_KeyPressed == ConvertKeyboardKeycode(key))
        m_KeyPressed = 0;
}

void Input::setJoystick(uint32_t code, bool state)
{
    m_Joystick[ConvertJoystickKeycode(code)] = state;
}

void Input::setPaddle(uint8_t paddle,bool is_first_user)
{
    m_Memory->dma_write_word((is_first_user) ? 0x9008 : 0x9009, paddle);
}

void Input::UpdateTimers()
{
    /*
    printf("%.2x\t%.2x\t%.2x\t%.4x\t%.4x\t%.2x\t%.2x\t%.2x\n",
        m_Memory->dma_read_word(VIA2::InterruptEnable),
        m_Memory->dma_read_word(VIA2::InterruptFlag),
        m_Memory->dma_read_word(VIA2::AuxiliaryRegister),
        m_Memory->dma_read_dword(VIA2::Timer1LOByte),
        m_Memory->dma_read_dword(VIA1::Timer2LOByte),
        0,0,0
        );*/
    uint8_t interrupt_enable = m_Memory->dma_read_word(VIA2::InterruptEnable);
    uint8_t interrupt_flags = m_Memory->dma_read_word(VIA2::InterruptFlag);
    uint8_t auxiliary_register = m_Memory->dma_read_word(VIA2::AuxiliaryRegister);

    uint16_t timer1 = m_Memory->dma_read_dword(VIA2::Timer1LOByte);
    uint16_t timer2 = m_Memory->dma_read_dword(VIA1::Timer2LOByte);

    if (timer1 > 0) 
        m_Memory->dma_write_dword(VIA2::Timer1LOByte, --timer1);
    if (timer2 > 0 && m_Memory->dma_read_word(VIA2::Timer2HIByte) != 0) 
        m_Memory->dma_write_dword(VIA1::Timer2LOByte, --timer2);

    if (timer1 == 0)
    {
        interrupt_flags |= 0x40;
        m_Memory->dma_write_word(VIA2::InterruptFlag, interrupt_flags);
    }

    if (timer2 == 0)
    {
        if (m_Memory->dma_read_word(VIA2::Timer2HIByte) != 0)
        {
            m_Memory->dma_write_dword(VIA1::Timer2LOByte, m_Memory->dma_read_dword(VIA2::Timer2LOByte));
            m_Memory->dma_write_dword(VIA2::Timer2HIByte, 0);

            interrupt_flags |= 0x20;
            m_Memory->dma_write_word(VIA2::InterruptFlag, interrupt_flags);
        }
    }

    if (interrupt_flags & interrupt_enable & 0x7F)
    {
        if (m_Cpu->m_Registers.getFlag(SR_INTERRUPTBIT) == false)
        {
            m_Cpu->interrupt(); 
            if (timer1 == 0)
            {
                if (auxiliary_register & 0x40)
                {
                    m_Memory->dma_write_dword(VIA2::Timer1LOByte, m_Memory->dma_read_dword(VIA2::Timer1LOByteLatch));
                }
                m_Memory->dma_write_word(VIA2::InterruptFlag, m_Memory->dma_read_word(VIA2::InterruptFlag) & ~0x40);
            }
            if (timer2 == 0)
            {
                m_Memory->dma_write_word(VIA2::InterruptFlag, m_Memory->dma_read_word(VIA2::InterruptFlag) & ~0x20);
            }
        }
    }
}

void Input::UpdateKeyboard()
{
    uint8_t portb = m_Memory->dma_read_word(VIA2::PortB);
    uint8_t portb_ddr = m_Memory->dma_read_word(VIA2::PortB_DDR);

    uint8_t key_column   = (m_KeyPressed >> 8) & 0xFF;
    uint8_t key_row = m_KeyPressed & 0xFF;

    if (portb == 0)
    {
        m_Memory->dma_write_word(VIA2::PortA, m_KeyPressed == 0 ? 0xff : key_row);
        m_Memory->dma_write_word(VIA2::PortAOutput, m_KeyPressed == 0 ? 0xff : key_row);
    }
    else
    {
        m_Memory->dma_write_word(VIA2::PortA, (portb == key_column) ? ((key_row == 0) ? 0xFF : key_row) : 0xFF);
        m_Memory->dma_write_word(VIA2::PortAOutput, (portb == key_column) ? ((key_row == 0) ? 0xFF : key_row) : 0xFF);
    }

    if ((portb == 0xF7) && m_ShiftPressed) {
        // Return row of shift key
        m_Memory->dma_write_word(VIA2::PortA, 0xFD & (m_KeyPressed == 0 ? 0xFF : (key_column == 0xF7 ? key_row : 0xFF)));
        m_Memory->dma_write_word(VIA2::PortAOutput, 0xFD & (m_KeyPressed == 0 ? 0xFF : (key_column == 0xF7 ? key_row : 0xFF)));
    }
    else if (portb == 0xDF && m_CBMPressed) {
        // Return row of cbm key
        m_Memory->dma_write_word(VIA2::PortA, 0xFE & (m_KeyPressed == 0 ? 0xFF : (key_column == 0xDF ? key_row : 0xFF)));
        m_Memory->dma_write_word(VIA2::PortAOutput, 0xFE & (m_KeyPressed == 0 ? 0xFF : (key_column == 0xDF ? key_row : 0xFF)));
    }

    //Joystick
    //reset
    m_Memory->dma_write_word(VIA1::PortA, 0xFF);
    m_Memory->dma_write_word(VIA1::PortAOutput, 0xFF);

    if (!(portb_ddr & 0x80))
    {
        m_Memory->dma_write_word(VIA2::PortB, m_Memory->dma_read_word(VIA2::PortB) | 0x80);
    }

    if (m_Joystick[0]) // Fire
    {
        m_Memory->dma_write_word(VIA1::PortA, m_Memory->dma_read_word(VIA1::PortA) & ~0x20);
        m_Memory->dma_write_word(VIA1::PortAOutput, m_Memory->dma_read_word(VIA1::PortA) & ~0x20);
    }

    if (m_Joystick[1]) // Up
    {
        m_Memory->dma_write_word(VIA1::PortA, m_Memory->dma_read_word(VIA1::PortA) & ~0x4);
        m_Memory->dma_write_word(VIA1::PortAOutput, m_Memory->dma_read_word(VIA1::PortA) & ~0x4);
    }

    if (m_Joystick[2]) // Right
    {
        m_Memory->dma_write_word(VIA2::PortB, m_Memory->dma_read_word(0x9120) & ~0x80);
    }

    if (m_Joystick[3]) // Down 
    {
        m_Memory->dma_write_word(VIA1::PortA, m_Memory->dma_read_word(VIA1::PortA) & ~0x8);
        m_Memory->dma_write_word(VIA1::PortAOutput, m_Memory->dma_read_word(VIA1::PortA) & ~0x8);
    }

    if (m_Joystick[4]) // Left
    {
        m_Memory->dma_write_word(VIA1::PortA, m_Memory->dma_read_word(VIA1::PortA) & ~0x10);
        m_Memory->dma_write_word(VIA1::PortAOutput, m_Memory->dma_read_word(VIA1::PortA) & ~0x10);
    }
}
