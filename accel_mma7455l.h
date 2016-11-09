#ifndef __ACCEL_MMA7455L_H__
#define __ACCEL_MMA7455L_H__

#include <inttypes.h>
#include <stdbool.h>

#define MMA7455L_I2C_ADDR  0x1D

#define MMA7455L_THRESHOLD  20

typedef struct {
  float x;
  float y;
  float z;
} MMA7455L_VALUES;

typedef struct {
  float roll;
  float pitch;
} MMA7455L_ANGLES;

#define MMA7455L_XOUTL 0x00
#define MMA7455L_XOUTH 0x01
#define MMA7455L_YOUTL 0x02
#define MMA7455L_YOUTH 0x03
#define MMA7455L_ZOUTL 0x04
#define MMA7455L_ZOUTH 0x05
#define MMA7455L_XOUT8 0x06
#define MMA7455L_YOUT8 0x07
#define MMA7455L_ZOUT8 0x08

#define MMA7455L_STATUS 0x09
#define MMA7455L_STATUS_DRDY (1 << 0)
#define MMA7455L_STATUS_DOVR (1 << 1)
#define MMA7455L_STATUS_PERR (1 << 2)

#define MMA7455L_DETSRC 0x0A
#define MMA7455L_DETSRC_INT2 (1 << 0)
#define MMA7455L_DETSRC_INT1 (1 << 1)
#define MMA7455L_DETSRC_PDZ (1 << 2)
#define MMA7455L_DETSRC_PDY (1 << 3)
#define MMA7455L_DETSRC_PDX (1 << 4)
#define MMA7455L_DETSRC_LDZ (1 << 5)
#define MMA7455L_DETSRC_LDY (1 << 6)
#define MMA7455L_DETSRC_LDX (1 << 7)

#define MMA7455L_TOUT 0x0B

#define MMA7455L_I2CAD 0x0D
#define MMA7455L_I2CAD_DAD_MASK 0x7F
#define MMA7455L_I2CAD_I2CDIS (1 << 7)

#define MMA7455L_USRINF 0x0E

#define MMA7455L_WHOAMI 0x0F

#define MMA7455L_XOFFL 0x10
#define MMA7455L_XOFFH 0x11
#define MMA7455L_YOFFL 0x12
#define MMA7455L_YOFFH 0x13
#define MMA7455L_ZOFFL 0x14
#define MMA7455L_ZOFFH 0x15

#define MMA7455L_MCTL 0x16
#define MMA7455L_MCTL_MODE_MASK 0x03
#define MMA7455L_MCTL_MODE_STANDBY (0 << 0)
#define MMA7455L_MCTL_MODE_MEASUREMENT (1 << 0)
#define MMA7455L_MCTL_MODE_LEVEL_DETECT (2 << 0)
#define MMA7455L_MCTL_MODE_PULSE_DETECT (3 << 0)
#define MMA7455L_MCTL_GLVL_MASK 0x0C
#define MMA7455L_MCTL_GLVL_8G (0 << 2)
#define MMA7455L_MCTL_GLVL_2G (1 << 1)
#define MMA7455L_MCTL_GLVL_4G (2 << 2)
#define MMA7455L_MCTL_STON (1 << 4)
#define MMA7455L_MCTL_SPI3W (1 << 5)
#define MMA7455L_MCTL_DRPD (1 << 6)
#define MMA7455L_MCTL_LPEN (1 << 7)

#define MMA7455L_SENSITIVITY_2G 1.0
#define MMA7455L_SENSITIVITY_4G 2.0
#define MMA7455L_SENSITIVITY_8G 4.0

#define MMA7455L_INTRST 0x17
#define MMA7455L_INTRST_CLRINT1 (1 << 0)
#define MMA7455L_INTRST_CLRINT2 (1 << 1)

#define MMA7455L_CTL1 0x18
#define MMA7455L_CTL1_INTPIN (1 << 0)
#define MMA7455L_CTL1_INTREG_MASK 0x06
#define MMA7455L_CTL1_INTREG_LEVEL_DETECT (0 << 1)
#define MMA7455L_CTL1_INTREG_PULSE_DETECT (1 << 1)
#define MMA7455L_CTL1_INTREG_SINGLE_DETECT (2 << 1)
#define MMA7455L_CTL1_XDA (1 << 3)
#define MMA7455L_CTL1_YDA (1 << 4)
#define MMA7455L_CTL1_ZDA (1 << 5)
#define MMA7455L_CTL1_THOPT (1 << 6)

#define MMA7455L_CTL2 0x19
#define MMA7455L_CTL2_LDPL (1 << 0)
#define MMA7455L_CTL2_PDPL (1 << 1)
#define MMA7455L_CTL2_DRVO (1 << 2)

#define MMA7455L_LDTH 0x1A
#define MMA7455L_PDTH 0x1B
#define MMA7455L_PW 0x1C
#define MMA7455L_LT 0x1D
#define MMA7455L_TW 0x1E

extern bool Mma7455l_Connected;
extern bool Mma7455l_Error;
extern volatile bool Mma7455l_MotionDetected;

extern MMA7455L_VALUES Mma7455l_AccelReal;
extern MMA7455L_ANGLES Mma7455l_AnglesReal;

bool Mma7455l_Init(void);
bool Mma7455l_Read(MMA7455L_VALUES* values);
bool Mma7455l_Get(MMA7455L_ANGLES* angles);

#endif // __ACCEL_MMA7455L__
