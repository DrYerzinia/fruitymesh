#include "ST95HF.h"

#define ST95HF_COMMAND     0x00
#define ST95HF_READ        0x02

ST95HF::ST95HF()
{
    //
}

void ST95HF::init(ST95HFPins p)
{

    pins = p;

    // Configure IOs
    FruityHal::SpiConfigureSlaveSelectPin(pins.ssPin);

    FruityHal::GpioConfigureOutput(pins.irqInPin);
    FruityHal::GpioPinSet(pins.irqInPin);

    FruityHal::GpioConfigureInput(pins.irqOutPin, FruityHal::GpioPullMode::GPIO_PIN_NOPULL);

    // Toggle IRQ down for 10uS
    FruityHal::GpioPinClear(pins.irqInPin);
    FruityHal::DelayUs(10);
    FruityHal::GpioPinSet(pins.irqInPin);

}

void ST95HF::command(ST95HF::Command cmd, u8 * txBuffer, int length)
{

    txBuffer[0] = ST95HF_COMMAND;
    txBuffer[1] = (u8) cmd;
    txBuffer[2] = length - 3;

    u8 rxBuffer[length];

    auto err = FruityHal::SpiTransfer(txBuffer, length, rxBuffer, pins.ssPin);

}

int ST95HF::read(u8 * rxBuffer, int bufferLength)
{

    u8 txBuffer[bufferLength];
    memset(txBuffer, 0, sizeof(txBuffer));
    txBuffer[0] = ST95HF_READ;

    int len = 3;

    if(bufferLength < 3){
        // TODO ERROR
    }

    FruityHal::GpioPinClear(pins.ssPin);

    FruityHal::SpiTransfer(txBuffer, len, rxBuffer, 31);
    int responseLength = rxBuffer[2];
    if(bufferLength < 3 + responseLength){
        // TODO ERROR
    }
    FruityHal::SpiTransfer(txBuffer + len, responseLength, rxBuffer + len, 31);
    len += responseLength;

    FruityHal::GpioPinSet(pins.ssPin);

    return len;

}