#include "main.h"
#include "usbd_cdc_if.h"
#include "TMAG5170.hpp"
#include <cstdio>
#include <cstring>
#include <stdint.h>

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

extern volatile uint8_t start_measurement_flag;
extern volatile uint8_t alert_flag;

void app_main(void) {
    TMAG5170 myTMAG;
    measure_frame frames[USB_BATCH_SIZE];

    myTMAG.init();

    while(1) {
        if(start_measurement_flag) {
            usb_rx_flag = 0;   // reset usb received flag

            uint8_t sr_byte = usb_rx_buf[1];
            uint8_t range_byte = usb_rx_buf[2];
            int32_t max_measurements = (usb_rx_buf[6] << 24) | (usb_rx_buf[5] << 16) | (usb_rx_buf[4] << 8) | usb_rx_buf[3];

            switch(sr_byte) {
            case 0:
                myTMAG.setConversionAverage(CONV_AVG_1x);
                break;
            case 1:
                myTMAG.setConversionAverage(CONV_AVG_2x);
                break;
            case 2:
                myTMAG.setConversionAverage(CONV_AVG_4x);
                break;
            case 3:
                myTMAG.setConversionAverage(CONV_AVG_8x);
                break;
            case 4:
                myTMAG.setConversionAverage(CONV_AVG_16x);
                break;
            default:
                myTMAG.setConversionAverage(CONV_AVG_32x);
            }

            switch(range_byte) {
            case 0:
                myTMAG.setMagneticRange(X_RANGE_75mT, Y_RANGE_75mT, Z_RANGE_75mT);
                break;
            case 1:
                myTMAG.setMagneticRange(X_RANGE_100mT, Y_RANGE_100mT, Z_RANGE_100mT);
                break;
            case 2:
                myTMAG.setMagneticRange(X_RANGE_150mT, Y_RANGE_150mT, Z_RANGE_150mT);
                break;
            default:
                myTMAG.setMagneticRange(X_RANGE_75mT, Y_RANGE_75mT, Z_RANGE_75mT);
            }

            CDC_Transmit_FS((uint8_t*)"D", 1);  // confirm setting

            myTMAG.enableMagneticChannel(true, true, true);
            myTMAG.setOperatingMode(OPERATING_MODE_ActiveMeasureMode);
            myTMAG.enableAlertOutput(true);

            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);

            int i = 0;
            int index;
            while(i < max_measurements && start_measurement_flag) {
                while(!alert_flag && start_measurement_flag);   // wait for alert flag to be set by EXTI interrupt
                if(!start_measurement_flag) break;
                alert_flag = 0;    // reset alert flag

                index = i % USB_BATCH_SIZE;
                // frames[i % USB_BATCH_SIZE].S.t1 = HAL_GetTick() * 1000; // temporary solution
                frames[index].S.t1 = GetMicros();
                frames[index].S.Bx = myTMAG.readXRaw();
                frames[index].S.By = myTMAG.readYRaw();
                frames[index].S.Bz = myTMAG.readZRaw();

                if((i + 1) % USB_BATCH_SIZE == 0) {   // send batch of measurements through USB
                    usb_tx_buf_len = USB_BATCH_SIZE * sizeof(measure_frame) + 1;
                    usb_tx_buf[0] = USB_BATCH_SIZE;  // first byte indicates number of frames in the batch
                    memcpy(usb_tx_buf + 1, frames, usb_tx_buf_len);
                    CDC_Transmit_FS(usb_tx_buf, usb_tx_buf_len);
                }

                i++;
            }

            if((i % USB_BATCH_SIZE) != 0) {     // deal with remaining unsent data
                usb_tx_buf_len = (i % USB_BATCH_SIZE) * sizeof(measure_frame) + 1;
                usb_tx_buf[0] = i % USB_BATCH_SIZE;
                memcpy(usb_tx_buf + 1, frames, usb_tx_buf_len);
                CDC_Transmit_FS(usb_tx_buf, usb_tx_buf_len);
            }

            myTMAG.enableMagneticChannel(false, false, false);
            myTMAG.setOperatingMode(OPERATING_MODE_ConfigurationMode);
            myTMAG.enableAlertOutput(false);

            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

            start_measurement_flag = 0;
        }
    }
}