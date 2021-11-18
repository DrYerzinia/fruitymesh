
#include "IIS2DH.h"

IIS2DH::IIS2DH(){
}

u8 IIS2DH::init(){

    u8 data[2];

    FruityHal::TwiRegisterRead(IIS2DH_ADDR, IIS2DH_REG__WHO_AM_I, data, 1);
    u8 chip_id = data[0];

    // 5.376kHz Low-power Mode XYZ Enabled
    data[0] = IIS2DH_REG__CTRL_REG_1;
    data[1] = 0x5F;
    FruityHal::TwiRegisterWrite(IIS2DH_ADDR, data, 2);

    // TODO High Pass Filter CTRL_REG_2

    data[0] = IIS2DH_REG__CLICK_THS;
    data[1] = 0x60;
    FruityHal::TwiRegisterWrite(IIS2DH_ADDR, data, 2);

    data[0] = IIS2DH_REG__TIME_LIMIT;
    data[1] = 0x05;
    FruityHal::TwiRegisterWrite(IIS2DH_ADDR, data, 2);

    data[0] = IIS2DH_REG__TIME_LATENCY;
    data[1] = 0x80;
    FruityHal::TwiRegisterWrite(IIS2DH_ADDR, data, 2);

    // Enable Z Single Click Interrupt
    data[0] = IIS2DH_REG__CLICK_CFG;
    data[1] = 0x10;
    FruityHal::TwiRegisterWrite(IIS2DH_ADDR, data, 2);

    return chip_id;

}

ErrorType IIS2DH::poll(u8 &click_src){

    u8 rxData[4];
    const auto err = FruityHal::TwiRegisterRead(IIS2DH_ADDR, IIS2DH_REG__CLICK_SRC, rxData, 1);
    if (err == ErrorType::FORBIDDEN || err == ErrorType::BUSY){
        return err;
    }

    click_src = rxData[0];

    return ErrorType::SUCCESS;

}