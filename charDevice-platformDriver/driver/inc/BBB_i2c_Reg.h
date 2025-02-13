#ifndef BBB_I2C_REG_H
#define BBB_I2C_REG_H

//Clock Module Peripheral (pagina 179) len = 1K = 0x400
#define CM_PER                                  0x44E00000
#define CM_PER_LEN                              0x00000400

//I2C clocks manager (pagina 1270)
#define IDCM_PER_I2C_CLKCTRL                    0X00000044
#define CM_PER_I2C2_CLKCTRL_MASK                0X00030003
#define CM_PER_I2C2_CLKCTRL_ENABLE              0X00000002

//Config de Control Module (pagina 180)
#define CTRL_MODULE_BASE                        0X44E10000
#define CTRL_MODULE_LEN                         0X00002000
#define CTRL_MODULE_UART1_CTSN                  0X000000978 //pin 20 = sda (1461)
#define CTRL_MODULE_UART1_RTSN                  0X00000097C //pin 19 = scl (1461)
#define CTRL_MODULE_UART1_MASK                  0X00000007F

//Capacidad de los pines (pagina 1515)
#define CTRL_MODULE_I2C_PINMODE                 0X00000023

//Config de Control Module (page 183)
#define I2C2                                    0x4819C000
#define I2C2_LEN                                0X00001000 //4K

//Registros específicos de I2C
#define IRQ_STATUS                              0X00000028
#define I2C_IRQSTATUS_CLR_ALL                   0X00006FFF
#define IRQ_XRDY_MASK                           0X00000010//4613  10000
#define IRQ_RRDY_MASK                           0X00000008//4613  01000
#define IRQ_ARDY_MASK                           0X00000004//4613  00100
#define I2C_IRQSTATUS_RAW                       0X00000024//4606            
    #define I2C_BUS_BUSY_MASK                   0x1000
#define I2C_IRQENABLE_SET                       0X0000002C//4614
#define I2C_IRQENABLE_SET_MASK                  0X00006FFF
#define I2C_IRQENABLE_SET_RX                    0X00000008
#define I2C_IRQENABLE_SET_TX                    0X00000010
#define I2C_IRQENABLE_CLR                       0X00000030//4616
#define I2C_IRQENABLE_CLR_MASK                  0X00006FFF
#define I2C_IRQENABLE_CLR_ALL                   0X00006FFF

#define I2C_CNT                                 0x00000098 // Page 4632
    #define I2C_CNT_MASK                        0x000000FF
#define I2C_DATA                                0x0000009C // Page 4633
#define I2C_SYSC                                0X10
#define I2C_IRQSTATUS                           0X28

#define I2C_CON                                 0x000000A4 // Page 4634
    #define I2C_CON_MASK                        0x0000BFF3
    #define I2C_CON_DISABLE                     0x00000000
    #define I2C_CON_TRX                         0x00000200
    #define I2C_CON_MST                         0x00000400
    #define I2C_CON_MASK_CLEAR_STT_STP          0xFFFFFFFC
    #define I2C_CON_STT                         0x00000001 // Page 4636
    #define I2C_CON_STP                         0x00000002 // Page 4636

#define I2C_OA                                  0x000000A8 // Page 4637
    #define I2C_OA_MASK                         0x000000FF
    #define I2C_OA_VALUE                        0x00000036 // dir random porque no uso multimaster

#define I2C_SA                                  0x000000AC // Page 4638
    #define I2C_SA_MASK                         0x000001FF
#define I2C_SA_ADDR                             0x068

#define I2C_PSC                                 0x000000B0 // Page 4639

#define I2C_SCLL                                0x000000B4 // Page 4640
    #define I2C_SCLL_MASK                       0x000000FF
    #define I2C_SCLL_400K                       0X09
    #define I2C_SCLL_100K                       0x00000035 // tLOW = 5 uS  = (SCLL+7)*(1/12MHz) => SCLL = 53

#define I2C_SCLH                                0x000000B8 // Page 4641
    #define I2C_SCLH_MASK                       0x000000FF
    #define I2C_SCLH_400K                       0X0B
    #define I2C_SCLH_100K                       0x00000037 // tHIGH = 5 uS = (SCLH+5)*(1/12MHz) => SCHH = 55

#define I2C_EN                                  0x8000
#define I2C_SYSTEST                             0XBC
#define I2C_BUFSTAT                             0XC0



#endif /*BBB_I2C_REG_H*/