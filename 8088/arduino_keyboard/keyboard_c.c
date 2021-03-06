#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

typedef uint8_t bool;
#define true 1
#define false 0

// 1 cycle @ 16MHz = 62.5ns

void raiseClock();
void lowerClock();
void raiseData();
void lowerData();
void setDataInput();
void setDataOutput();
void setClockInput();
void setClockOutput();
void wait2us();
void wait50us();
void wait1ms();
void wait250ms();
bool getData();
bool getClock();
void reset();
uint8_t receiveKeyboardByte();

extern uint8_t asciiToScancodes[0x5f] PROGMEM;
extern uint16_t remoteCodes[0x51] PROGMEM;
extern uint8_t defaultProgram[2] PROGMEM;

uint8_t serialBuffer[0x100];
uint8_t keyboardBuffer[0x100];
uint8_t programBuffer[0x400];

volatile uint8_t serialBufferPointer;
volatile uint16_t serialBufferCharacters;
volatile uint8_t keyboardBufferPointer;
volatile uint16_t keyboardBufferCharacters;

volatile bool spaceAvailable = true;

volatile bool shift = false;
volatile bool ctrl = false;
volatile bool alt = false;
volatile bool asciiMode = false;
volatile bool testerMode = true;
volatile bool receivedEscape = false;
volatile bool receivedXOff = false;
volatile bool sentXOff = false;
volatile bool needXOff = false;
volatile bool needXOn = false;
volatile uint8_t rawBytesRemaining = 0;
volatile uint16_t programCounter = 0;
volatile uint16_t programBytes = 0;
volatile uint16_t programBytesRemaining = 0;
volatile bool ramProgram = false;
volatile bool expectingRawCount = false;
volatile bool sentEscape = false;
volatile bool checkSum = 0;
volatile bool expectingCheckSum = false;
volatile bool testerRaw = false;
volatile bool remoteMode = false;

void enqueueSerialByte(uint8_t byte);

void sendSerialByte()
{
    if (!spaceAvailable)
        return;
    // We should be able to send XOn/XOff even if we've received XOff.
    if (!needXOff && !needXOn && receivedXOff)
        return;
    uint8_t c;
    if (needXOff) {
        c = 19;
        sentXOff = true;
        needXOff = false;
    }
    else {
        if (needXOn) {
            c = 17;
            sentXOff = false;
            needXOn = false;
        }
        else {
            if (serialBufferCharacters == 0) {
                // There's nothing we need to send!
                return;
            }
            c = serialBuffer[serialBufferPointer];
            if (c == 0 || c == 17 || c == 19) {
                if (!sentEscape) {
                    c = 0;
                    sentEscape = true;
                }
                else {
                    sentEscape = false;
                    ++serialBufferPointer;
                    --serialBufferCharacters;
                }
            }
            else {
                ++serialBufferPointer;
                --serialBufferCharacters;
            }
        }
    }
    // Actually send the byte
    UDR0 = c;
    spaceAvailable = false;
}

void enqueueSerialByte(uint8_t byte)
{
    serialBuffer[(serialBufferPointer + serialBufferCharacters) & 0xff] =
        byte;
    ++serialBufferCharacters;
    sendSerialByte();
}

void print(const char* s)
{
    do {
        char c = pgm_read_byte(s++);
        if (c == 0)
            break;
        if (c == '\n')
            enqueueSerialByte('\r');
        enqueueSerialByte(c);
    } while (true);
}

void enqueueKeyboardByte(uint8_t byte)
{
    keyboardBuffer[(keyboardBufferPointer + keyboardBufferCharacters) & 0xff]
        = byte;
    ++keyboardBufferCharacters;
    // If our buffer is getting too full, tell the host to stop sending.
    if (keyboardBufferCharacters >= 0xf0 && !sentXOff) {
        needXOff = true;
        sendSerialByte();
    }
}

bool processCommand(uint8_t command)
{
    switch (command) {
        case 1:
            testerMode = false;
            return true;
        case 2:
            testerMode = true;
            return true;
        case 3:
            ramProgram = true;
            programBytesRemaining = 0xffff;
            return true;
        case 4:
            ramProgram = false;
            return true;
        case 5:
            expectingRawCount = true;
            return true;
        case 6:
            keyboardBufferCharacters = 0;
            return true;
        case 7:
            print(PSTR("Resetting\n"));
            reset();
            return true;
        case 8:
            testerRaw = true;
            return true;
        case 9:
            testerRaw = false;
            return true;
        case 0xa:
            remoteMode = true;
            asciiMode = false;
            return true;
        case 0xb:
            remoteMode = false;
            return true;
    }
    return false;
}

volatile uint16_t pulseSpaceDuration = 0xffff;
volatile uint8_t pulseSpace = 0;
volatile uint8_t waiting = 0;

SIGNAL(TIMER1_OVF_vect)
{
    PORTB = (PORTB & 0xf7) | pulseSpace;
    ICR1 = pulseSpaceDuration;
    waiting = 0;
}

void sendPulse(uint16_t microseconds)
{
    waiting = 1;
    while (waiting != 0);
    pulseSpaceDuration = microseconds*2;
    pulseSpace = 8;
}

void sendSpace(uint16_t microseconds)
{
    waiting = 1;
    while (waiting != 0);
    pulseSpaceDuration = microseconds*2;
    pulseSpace = 0;
}

void sendRemoteData(uint16_t data, int bits)
{
    for (int i = 0; i < bits; i++) {
        sendPulse(553);
        if (data & 0x8000)
            sendSpace(1648);
        else
            sendSpace(518);
        data <<= 1;
    }
}

void sendRemoteCode(uint16_t code)
{
    sendPulse(9016);
    sendSpace(4447);
    sendRemoteData(0xFF, 16);
    sendRemoteData(code, 16);
    sendPulse(555);
    sendSpace(1000);
}

volatile uint16_t remoteCode = 0;

void processCharacter(uint8_t received)
{
    if (received == 0 && !receivedEscape) {
        receivedEscape = true;
        return;
    }
    if ((received == 17 || received == 19) && !receivedEscape) {
        receivedXOff = (received == 19);
        enqueueSerialByte(receivedXOff ? 'F' : 'N');
        receivedEscape = false;
        return;
    }
    receivedEscape = false;

    if (expectingRawCount) {
        rawBytesRemaining = received;
        checkSum = received;
        expectingRawCount = false;
        return;
    }
    if (rawBytesRemaining > 0) {
        enqueueKeyboardByte(received);
        checkSum += received;
        --rawBytesRemaining;
        if (rawBytesRemaining == 0)
            expectingCheckSum = true;
        return;
    }
    if (expectingCheckSum) {
        enqueueSerialByte(received == checkSum ? 'K' : '~');
        expectingCheckSum = false;
        return;
    }
    if (programBytesRemaining == 0xffff) {
        programBytes = received;
        --programBytesRemaining;
        return;
    }
    if (programBytesRemaining == 0xfffe) {
        programBytes |= received << 8;
        programBytesRemaining = programBytes;
        enqueueSerialByte('p');
        return;
    }
    if (programBytesRemaining > 0) {
        programBuffer[programBytes - programBytesRemaining] = received;
        --programBytesRemaining;
        if (programBytesRemaining == 0)
            enqueueSerialByte('d');
        return;
    }
    if (received == 0) {
        asciiMode = true;
        return;
    }
    if (received == 0x7f) {
        asciiMode = false;
        return;
    }
    if (!asciiMode) {
        if (!processCommand(received - 0x70)) {
            if (remoteMode) {
                if (received < 0x51)
                    remoteCode = pgm_read_word(&remoteCodes[received]);
            }
            else
                enqueueKeyboardByte(received);
        }
        return;
    }
    if (!processCommand(received)) {
        uint8_t scanCode;
        switch (received) {
            case 8:
                scanCode = 0x0e;
                break;
            case 9:
                scanCode = 0x0f;
                break;
            case 10:
                // We handle LF and CRLF line endings by treating LF
                // as a press of Enter and ignoring CR. Revisit if
                // this causes problems.
                scanCode = 0x1c;
                break;
            case 27:
                scanCode = 0x01;
                break;
            default:
                if (received >= 0x20 && received <= 0x7e) {
                    scanCode = pgm_read_byte(
                        &asciiToScancodes[received - 0x20]);
                }
                else {
                    // Handle invalid ASCII codes by ignoring them.
                    return;
                }
                // TODO: Add codes for cursor movement, function keys
                // etc.
        }
        bool shifted = (scanCode & 0x80) != 0;
        if (shifted != shift) {
            // We always use the left shift key for typing shifted
            // characters.
            enqueueKeyboardByte(0x2a | (shifted ? 0 : 0x80));
            shift = shifted;
        }
        enqueueKeyboardByte(scanCode);
        enqueueKeyboardByte(scanCode | 0x80);
    }
}

SIGNAL(USART_RX_vect)
{
    processCharacter(UDR0);
}

SIGNAL(USART_TX_vect)
{
    spaceAvailable = true;
    sendSerialByte();
}

void clearInterruptedKeystroke()
{
    setClockOutput();
    while (getData()) {
        lowerClock();
        wait50us();
        raiseClock();
        wait50us();
    }
    setClockInput();
}

void sendKeyboardBit(uint8_t bit)
{
    if (bit != 0)
        raiseData();
    else
        lowerData();
    wait50us();
    lowerClock();
    wait50us();
    raiseClock();
}

bool sendKeyboardByte(uint8_t data)
{
    // We read the clock as high immediately before entering this routine.
    // The XT keyboard hardware holds the data line low to signal that the
    // previous byte has not yet been acknowledged by software.
    while (!getData()) { }
    if (!getClock()) {
        // Uh oh - the clock went low - the XT wants something (send byte or
        // reset). This should never happen during a reset, so we can just
        // abandon this byte.
        return false;
    }
    setClockOutput();
    setDataOutput();
    sendKeyboardBit(0);
    sendKeyboardBit(1);
    for (uint8_t i = 0; i < 8; ++i) {
        sendKeyboardBit(data & 1);
        data >>= 1;
    }
    raiseData();
    setClockInput();
    setDataInput();
    if (!getClock()) {
        // The clock went low while we were sending - retry this byte once
        // the reset or send-requested condition has been resolved.
        return false;
    }
    // The byte went through.
    return true;
}

volatile uint8_t countdown = 0;

SIGNAL(TIMER0_OVF_vect)
{
    if (countdown > 0)
        --countdown;
}

int main()
{
    // Initialize hardware ports

    // DDRB value:   0x08  (Port B Data Direction Register)
    //   DDB0           0  Data                        - input
    //   DDB1           0  Clock                       - input
    //   DDB2           0  Reset                       - input
    //   DDB3           8  Remote LED                  - output
    //   DDB4           0
    //   DDB5           0
    //   DDB6           0
    //   DDB7           0
    DDRB = 0x08;

    // PORTB value:  0x03  (Port B Data Register)
    //   PORTB0         1  Data                        - high
    //   PORTB1         2  Clock                       - high
    //   PORTB2         0  Reset                       - low
    //   PORTB3         0  Remote LED                  - low
    //   PORTB4         0
    //   PORTB5         0
    //   PORTB6         0
    //   PORTB7         0
    PORTB = 0x03;

    DDRD |= 0x60;

    // TCCR0A value: 0x03  (Timer/Counter 0 Control Register A)
    //   WGM00          1  } Waveform Generation Mode = 7 (Fast PWM, TOP=OCRA)
    //   WGM01          2  }
    //
    //
    //   COM0B0         0  } Compare Output Mode for Channel B: normal operation
    //   COM0B1         0  }
    //   COM0A0         0  } Compare Output Mode for Channel A: normal operation
    //   COM0A1         0  }
    TCCR0A = 0x03;

    // TCCR0B value: 0x04  (Timer/Counter 0 Control Register B)
    //   CS00           0  } Clock select: clkIO/256
    //   CS01           0  }
    //   CS02           4  }
    //   WGM02          0  Waveform Generation Mode = 3 (Fast PWM, TOP=0xff)
    //
    //
    //   FOC0B          0  Force Output Compare B
    //   FOC0A          0  Force Output Compare A
    TCCR0B = 0x04;

    // OCR0A value: 16MHz / (125*256) = 500Hz
    OCR0A = 125;

    // TIMSK0 value: 0x01  (Timer/Counter 0 Interrupt Mask Register)
    //   TOIE0          1  Timer 0 overflow:  no interrupt
    //   OCIE0A         0  Timer 0 compare A: no interrupt
    //   OCIE0B         0  Timer 0 compare B: no interrupt
    TIMSK0 = 0x01;

    // TIMSK1 value: 0x01  (Timer/Counter 1 Interrupt Mask Register)
    //   TOIE1          1  Timer 1 overflow:  interrupt
    //   OCIE1A         0  Timer 1 compare A: no interrupt
    //   OCIE1B         0  Timer 1 compare B: no interrupt
    //
    //
    //   ICIE1          0  Timer 1 input capture: no interrupt
    TIMSK1 = 0x01;

    // TCCR1A value: 0x02  (Timer/Counter 1 Control Register A)
    //   WGM10          0  } Waveform Generation Mode = 14 (Fast PWM, TOP=ICR1)
    //   WGM11          2  }
    //
    //
    //   COM1B0         0  } Compare Output Mode for Channel B: normal port operation, OC1B disconnected
    //   COM1B1         0  }
    //   COM1A0         0  } Compare Output Mode for Channel A: normal port operation, OC1B disconnected
    //   COM1A1         0  }
    TCCR1A = 0x02;

    // TCCR1B value: 0x1a  (Timer/Counter 1 Control Register B)
    //   CS10           0  } Clock select: clkIO/8 (from prescaler)
    //   CS11           2  }
    //   CS12           0  }
    //   WGM12          8  } Waveform Generation Mode = 14 (Fast PWM, TOP=ICR1)
    //   WGM13       0x10  }
    //
    //   ICES1          0  Input Capture Edge Select: falling
    //   ICNC1          0  Input Capture Noise Canceler: disabled
    TCCR1B = 0x1a;

    // TCCR1C value: 0x00  (Timer/Counter 1 Control Register C)
    //
    //
    //
    //
    //
    //
    //   FOC1B          0  Force Output Compare for Channel B
    //   FOC1A          0  Force Output Compare for Channel A
    TCCR1C = 0x00;

    // UCSR0A value: 0x02  (USART Control and Status Register 0 A)
    //   MPCM0          0  Multi-processor Communcation Mode: disabled
    //   U2X0           2  Double the USART Transmission Speed: disabled
    //
    //
    //
    //
    //   TXC0           0  USART Transmit Complete: not cleared
    UCSR0A = 0x02;

    // UCSR0B value: 0xd8  (USART Control and Status Register 0 B)
    //   TXB80          0  Transmit Data Bit 8 0
    //
    //   UCSZ02         0  Character Size 0: 8 bit
    //   TXEN0          8  Transmitter Enable 0: enabled
    //   RXEN0       0x10  Receiver Enable 0: enabled
    //   UDRIE0         0  USART Data Register Empty Interrupt Enable 0:
    //                       disabled
    //   TXCIE0      0x40  TX Complete Interrupt Enable 0: enabled
    //   RXCIE0      0x80  RX Complete Interrupt Enable 0: enabled
    UCSR0B = 0xd8;

    // UCSR0C value: 0x06  (USART Control and Status Register 0 C)
    //   UCPOL0         0  Clock Polarity
    //   UCSZ00         2  Character Size: 8 bit
    //   UCSZ01         4  Character Size: 8 bit
    //   USBS0          0  Stop Bit Select: 1-bit
    //   UPM00          0  Parity Mode: disabled
    //   UPM01          0  Parity Mode: disabled
    //   UMSEL00        0  USART Mode Select: asynchronous
    //   UMSEL01        0  USART Mode Select: asynchronous
    UCSR0C = 0x06;

    // UBRR0L value:       (USART Baud Rate Register Low)
//    UBRR0L = 0x10;  // 115200
//    UBRR0L = 0x33;  // 38400
    UBRR0L = 0x67;  // 19200

    // UBRR0H value: 0x00  (USART Baud Rate Register High)
    UBRR0H = 0x00;

    sei();

    print(PSTR("Arduino keyboard 20121103\n"));
    print(PSTR("Kernel version "));
    print((const char*)defaultProgram + 4);
    print(PSTR("\n"));

    // All the keyboard interface stuff is done on the main thread.
    do {
        if (!getClock()) {
            wait1ms();
            if (!getClock()) {
                // If the clock line is held low for this long it means the XT
                // is resetting the keyboard.
                while (!getClock()) { }  // Wait for clock to go high again.
                // There are 4 different things the BIOS recognizes here:
                // 0xaa - a keyboard
                // 0x65 - tester doodad: download the code from it and run it.
                // 0x00 - boot in test mode - this affects the diagnostic
                //   beeps and flashes an LED via the keyboard clock line. I
                //   don't think it's useful for this program.
                // Everything else - keyboard error. Also not useful.
                if (testerMode) {
                    // We assume the BIOS won't be foolish enough to pull the
                    // clock line low during a reset. If it does the data will
                    // be corrupted as we don't attempt to remember where we
                    // got to and retry from there.
                    sendKeyboardByte(0x65);
                    if (!testerRaw) {
                        if (ramProgram) {
                            sendKeyboardByte(programBytes & 0xff);
                            sendKeyboardByte(programBytes >> 8);
                            for (uint16_t i = 0; i < programBytes; ++i)
                                sendKeyboardByte(programBuffer[i]);
                        }
                        else {
                            uint16_t programBytes =
                                pgm_read_byte(&defaultProgram[0]);
                            programBytes |=
                                (uint16_t)(pgm_read_byte(&defaultProgram[1]))
                                << 8;
                            sendKeyboardByte(programBytes & 0xff);
                            sendKeyboardByte(programBytes >> 8);
                            for (uint16_t i = 0; i < programBytes; ++i)
                                sendKeyboardByte(
                                    pgm_read_byte(&defaultProgram[i+2]));
                        }
                    }
                }
                else {
                    sendKeyboardByte(0xaa);
                    wait2us();
                    while (!getData()) { }
                    // If we send anything in the first 250ms after a reset,
                    // the BIOS will assume it's a stuck key. However, we
                    // can't just wait around for 250ms because the BIOS does
                    // two resets and doesn't wait 250ms after the first, so
                    // if we wait synchronously we'll miss the second reset.
                    // Hence we wait asynchronously.
                    countdown = 125;
                }
                // End of reset code
            }
            else {
                enqueueSerialByte('#');
                while (!getClock()) { }  // Wait for clock to go high again.
                // A short clock-low pulse. This is the XT trying to send us
                // some data.
                clearInterruptedKeystroke();
                // Send the number of bytes that the XT can safely send us.
                cli();
                sendKeyboardByte(serialBufferCharacters == 0 ? 255 :
                    256-serialBufferCharacters);
                sei();
                uint8_t count = receiveKeyboardByte();
                for (uint8_t i = 0; i < count; ++i) {
                    cli();
                    enqueueSerialByte(receiveKeyboardByte());
                    sei();
                }
            }
        }
        else {
            // Clock is high - we're free to send if we have data.
            if (countdown == 0 && keyboardBufferCharacters != 0) {
                if (sendKeyboardByte(keyboardBuffer[keyboardBufferPointer])) {
                    // Successfully sent - remove this character from the
                    // buffer. This needs to be done with interrupts off in
                    // case a character is received (and added to the buffer)
                    // while these instruction are being processed (even a
                    // an 8-bit memory increment is not atomic on 8-bit AVR).
                    cli();
                    ++keyboardBufferPointer;
                    --keyboardBufferCharacters;
                    sei();
                    // If we've made enough space in the buffer, allow
                    //  receiving again.
                    if (keyboardBufferCharacters < 0xf0 && sentXOff) {
                        needXOn = true;
                        sendSerialByte();
                    }
                }
            }
            if (remoteCode != 0) {
                sendRemoteCode(remoteCode);
                remoteCode = 0;
            }
        }
    } while (true);
}
