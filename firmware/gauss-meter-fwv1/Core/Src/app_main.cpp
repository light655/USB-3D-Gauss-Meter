#include "main.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "usbd_cdc_if.h"
#include "app_main.h"
#include "TMAG5170.hpp"
#include <cstdio>
#include <cstring>
#include <stdint.h>

#define USB_BUF_LEN 128

union measure_frame {   // frame to hold 1 measurement data
    struct {
        uint32_t t1;
        int16_t Bx;
        int16_t By;
        int16_t Bz;
    } S;
    char arr[10];       // unioned with char array for easier iteration when sending data through USB
};

extern uint8_t usb_tx_buf[USB_BUF_LEN];
extern uint16_t usb_tx_buf_len;

extern uint8_t usb_rx_buf[USB_BUF_LEN];
extern uint16_t usb_rx_buf_len;
extern uint8_t usb_rx_flag;

extern volatile uint8_t alert_flag;

void app_main(void) {
    TMAG5170 myTMAG;
    measure_frame frames[100];

    myTMAG.init();
    myTMAG.setConversionAverage(CONV_AVG_32x);
    myTMAG.enableMagneticChannel(true, true, true);
    myTMAG.setMagneticRange(X_RANGE_300mT, Y_RANGE_300mT, Z_RANGE_300mT);

    while(1) {
        if(usb_rx_flag) {
            usb_rx_flag = 0;    // clear flag

            if(usb_rx_buf[0] == 'S') {  // start signal from python client
                int num_measure = (usb_rx_buf[4] << 24) | (usb_rx_buf[3] << 16) | (usb_rx_buf[2] << 8) | (usb_rx_buf[1]);
                
                myTMAG.setOperatingMode(OPERATING_MODE_ActiveMeasureMode);
                myTMAG.enableAlertOutput(true);
                
                int i = 0;
                while(i < num_measure) {
                    if(alert_flag) {    // wait for GPIO interrupt
                        frames[i].S.t1 = HAL_GetTick() * 1000;  // temporary solution 
                        frames[i].S.Bx = myTMAG.readXRaw();
                        frames[i].S.By = myTMAG.readYRaw();
                        frames[i].S.Bz = myTMAG.readZRaw();

                        i++;
                        alert_flag = 0; // clear GPIO interrupt flag
                    }
                }

                myTMAG.setOperatingMode(OPERATING_MODE_ConfigurationMode);
                myTMAG.enableAlertOutput(false);

                for(i = 0; i < num_measure; i++) {
                    std::memcpy(usb_tx_buf, frames[i].arr, 10);
                    CDC_Transmit_FS(usb_tx_buf, 10);
                    HAL_Delay(1);
                }
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
                HAL_Delay(100);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
            } else if(usb_rx_buf[0] == 'R') {   // change range signal
                switch(usb_rx_buf[1]) {         // range byte
                    case 0:
                        myTMAG.setMagneticRange(X_RANGE_75mT, Y_RANGE_75mT, Z_RANGE_75mT);
                        break;
                    case 1:
                        myTMAG.setMagneticRange(X_RANGE_150mT, Y_RANGE_150mT, Z_RANGE_150mT);
                        break;
                    case 2:
                        myTMAG.setMagneticRange(X_RANGE_300mT, Y_RANGE_300mT, Z_RANGE_300mT);
                        break;
                    default:
                        break;
                }

                uint8_t temp[10] = "D";
                CDC_Transmit_FS(temp, 1);

                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
                HAL_Delay(100);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
            }
        }
    }
}
