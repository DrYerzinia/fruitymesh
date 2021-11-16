#pragma once

#include "FruityHal.h"
#include "PrimitiveTypes.h"

class ST95HF {

    public:

        enum class Command : u8
        {
            IDN = 0x01,
        };

        ST95HF();

        void init(ST95HFPins p);
        void command(ST95HF::Command cmd, u8 * txBuffer, int length);
        int read(u8 * rxBuffer, int bufferLength);

        ST95HFPins pins;

};