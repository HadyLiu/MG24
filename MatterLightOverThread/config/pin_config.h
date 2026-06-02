#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// $[CMU]
// [CMU]$

// $[LFXO]
// [LFXO]$

// $[PRS.ASYNCH0]
// [PRS.ASYNCH0]$

// $[PRS.ASYNCH1]
// [PRS.ASYNCH1]$

// $[PRS.ASYNCH2]
// [PRS.ASYNCH2]$

// $[PRS.ASYNCH3]
// [PRS.ASYNCH3]$

// $[PRS.ASYNCH4]
// [PRS.ASYNCH4]$

// $[PRS.ASYNCH5]
// [PRS.ASYNCH5]$

// $[PRS.ASYNCH6]
// [PRS.ASYNCH6]$

// $[PRS.ASYNCH7]
// [PRS.ASYNCH7]$

// $[PRS.ASYNCH8]
// [PRS.ASYNCH8]$

// $[PRS.ASYNCH9]
// [PRS.ASYNCH9]$

// $[PRS.ASYNCH10]
// [PRS.ASYNCH10]$

// $[PRS.ASYNCH11]
// [PRS.ASYNCH11]$

// $[PRS.ASYNCH12]
// [PRS.ASYNCH12]$

// $[PRS.ASYNCH13]
// [PRS.ASYNCH13]$

// $[PRS.ASYNCH14]
// [PRS.ASYNCH14]$

// $[PRS.ASYNCH15]
// [PRS.ASYNCH15]$

// $[PRS.SYNCH0]
// [PRS.SYNCH0]$

// $[PRS.SYNCH1]
// [PRS.SYNCH1]$

// $[PRS.SYNCH2]
// [PRS.SYNCH2]$

// $[PRS.SYNCH3]
// [PRS.SYNCH3]$

// $[GPIO]
// [GPIO]$

// $[TIMER0]
// [TIMER0]$

// $[TIMER1]
// TIMER1 CC0 on PA03
#ifndef TIMER1_CC0_PORT                         
#define TIMER1_CC0_PORT                          SL_GPIO_PORT_A
#endif
#ifndef TIMER1_CC0_PIN                          
#define TIMER1_CC0_PIN                           3
#endif

// [TIMER1]$

// $[TIMER2]
// [TIMER2]$

// $[TIMER3]
// TIMER3 CC0 on PD01
#ifndef TIMER3_CC0_PORT                         
#define TIMER3_CC0_PORT                          SL_GPIO_PORT_D
#endif
#ifndef TIMER3_CC0_PIN                          
#define TIMER3_CC0_PIN                           1
#endif

// [TIMER3]$

// $[TIMER4]
// TIMER4 CC0 on PB02
#ifndef TIMER4_CC0_PORT                         
#define TIMER4_CC0_PORT                          SL_GPIO_PORT_B
#endif
#ifndef TIMER4_CC0_PIN                          
#define TIMER4_CC0_PIN                           2
#endif

// TIMER4 CC1 on PB03
#ifndef TIMER4_CC1_PORT                         
#define TIMER4_CC1_PORT                          SL_GPIO_PORT_B
#endif
#ifndef TIMER4_CC1_PIN                          
#define TIMER4_CC1_PIN                           3
#endif

// TIMER4 CC2 on PB04
#ifndef TIMER4_CC2_PORT                         
#define TIMER4_CC2_PORT                          SL_GPIO_PORT_B
#endif
#ifndef TIMER4_CC2_PIN                          
#define TIMER4_CC2_PIN                           4
#endif

// [TIMER4]$

// $[USART0]
// [USART0]$

// $[I2C1]
// [I2C1]$

// $[EUSART1]
// EUSART1 RX on PB00
#ifndef EUSART1_RX_PORT                         
#define EUSART1_RX_PORT                          SL_GPIO_PORT_B
#endif
#ifndef EUSART1_RX_PIN                          
#define EUSART1_RX_PIN                           0
#endif

// EUSART1 SCLK on PB01
#ifndef EUSART1_SCLK_PORT                       
#define EUSART1_SCLK_PORT                        SL_GPIO_PORT_B
#endif
#ifndef EUSART1_SCLK_PIN                        
#define EUSART1_SCLK_PIN                         1
#endif

// EUSART1 TX on PD00
#ifndef EUSART1_TX_PORT                         
#define EUSART1_TX_PORT                          SL_GPIO_PORT_D
#endif
#ifndef EUSART1_TX_PIN                          
#define EUSART1_TX_PIN                           0
#endif

// [EUSART1]$

// $[KEYSCAN]
// [KEYSCAN]$

// $[LETIMER0]
// [LETIMER0]$

// $[IADC0]
// IADC0 SCAN0POS on PA08
#ifndef IADC0_SCAN0POS_PORT                     
#define IADC0_SCAN0POS_PORT                      SL_GPIO_PORT_A
#endif
#ifndef IADC0_SCAN0POS_PIN                      
#define IADC0_SCAN0POS_PIN                       8
#endif

// IADC0 SCAN10POS on PC04
#ifndef IADC0_SCAN10POS_PORT                    
#define IADC0_SCAN10POS_PORT                     SL_GPIO_PORT_C
#endif
#ifndef IADC0_SCAN10POS_PIN                     
#define IADC0_SCAN10POS_PIN                      4
#endif

// IADC0 SCAN11POS on PC05
#ifndef IADC0_SCAN11POS_PORT                    
#define IADC0_SCAN11POS_PORT                     SL_GPIO_PORT_C
#endif
#ifndef IADC0_SCAN11POS_PIN                     
#define IADC0_SCAN11POS_PIN                      5
#endif

// [IADC0]$

// $[ACMP0]
// [ACMP0]$

// $[ACMP1]
// [ACMP1]$

// $[VDAC0]
// [VDAC0]$

// $[VDAC1]
// [VDAC1]$

// $[PCNT0]
// [PCNT0]$

// $[HFXO0]
// [HFXO0]$

// $[I2C0]
// [I2C0]$

// $[EUSART0]
// EUSART0 RX on PA06
#ifndef EUSART0_RX_PORT                         
#define EUSART0_RX_PORT                          SL_GPIO_PORT_A
#endif
#ifndef EUSART0_RX_PIN                          
#define EUSART0_RX_PIN                           6
#endif

// EUSART0 TX on PA07
#ifndef EUSART0_TX_PORT                         
#define EUSART0_TX_PORT                          SL_GPIO_PORT_A
#endif
#ifndef EUSART0_TX_PIN                          
#define EUSART0_TX_PIN                           7
#endif

// [EUSART0]$

// $[PTI]
// [PTI]$

// $[MODEM]
// [MODEM]$

// $[CUSTOM_PIN_NAME]
// [CUSTOM_PIN_NAME]$


#endif // PIN_CONFIG_H


