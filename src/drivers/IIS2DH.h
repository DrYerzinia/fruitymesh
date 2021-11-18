#pragma once

#include "FruityHal.h"

#define IIS2DH_ADDR                 0x19

#define IIS2DH_REG__WHO_AM_I        0x0F
#define IIS2DH_REG__CTRL_REG_1      0x20
#define IIS2DH_REG__CTRL_REG_2      0x21
#define IIS2DH_REG__CTRL_REG_3      0x22
#define IIS2DH_REG__CTRL_REG_4      0x23
#define IIS2DH_REG__CTRL_REG_5      0x24
#define IIS2DH_REG__CTRL_REG_6      0x26
#define IIS2DH_REG__CLICK_CFG       0x38
#define IIS2DH_REG__CLICK_SRC       0x39
#define IIS2DH_REG__CLICK_THS       0x3A
#define IIS2DH_REG__TIME_LIMIT      0x3B
#define IIS2DH_REG__TIME_LATENCY    0x3C
#define IIS2DH_REG__TIME_WINDOW     0x3D

class IIS2DH
{
public:

    IIS2DH();

    u8 init();

    ErrorType poll(u8 &click_src);

};