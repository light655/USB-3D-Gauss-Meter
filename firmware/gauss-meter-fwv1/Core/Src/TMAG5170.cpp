/**
 * @file TMAG5170.cpp
 * @author Chen, Liang-Yu
 * @brief TMAG5170 library for STM32
 * @version 0.1
 * @date 2026-01-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "TMAG5170.hpp"
#include "stm32f103xb.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_spi.h"

/**
 * @brief Construct a new TMAG5170::TMAG5170 object
 * 
 */
TMAG5170::TMAG5170(void) {

    return;
}

/**
 * @brief Generate CRC for SPI communication.
 * 
 * @param data 32-bit data to generate CRC for. The last four bits should be all 0's.
 * @return uint32_t containing the four CRC bits.
 */
uint32_t TMAG5170::generateCRC(uint32_t data) {
    uint32_t TMAG5170_CRC = 0xf;     // least significant 4 bits are the shift register

    for(int i = 0; i < 32; i++) {
        uint32_t inv = ((data & 0x80000000) >> 31) ^ ((TMAG5170_CRC & 0x8) >> 3);
            // XOR MSB of data and shift register
        uint32_t poly = (inv << 1) | inv;
            // polynomial = x^4 + x + 1
        uint32_t XORed = (TMAG5170_CRC << 1) ^ poly;
        TMAG5170_CRC = XORed & 0xf;
        data <<= 1;
    }    

    return TMAG5170_CRC;
}

/**
 * @brief Check the CRC of the received frame.
 * 
 * @param received_frame 32-bit data of the received frame.
 * @return Return nonzero value if CRC is incorrect. 
 */
int TMAG5170::checkCRC(uint32_t received_frame) {
    uint32_t received_CRC_calculated = generateCRC(received_frame & 0xfffffff0);
        // clear the CRC bits and calculate the CRC
    if(received_CRC_calculated != (received_frame & 0xf)) {
        // calculated CRC does not match the actual received bits
        return 1;
    } else {
        return 0;
    }
}

/**
 * @brief Exchange an SPI frame with TMAG5170.
 * 
 * @param frame 32-bit data of the frame to send. CRC needs to be included in this frame.
 * @return uint32_t containing the received frame. CRC is not checked for the received frame.
 */
uint32_t TMAG5170::exchangeFrame(uint32_t frame) {
    TMAG5170_SPI_frame send_frame, receive_frame;
    send_frame.data32 = __builtin_bswap32(frame);   // swap the bytes to match the order when viewed as array

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, send_frame.byte_arr, receive_frame.byte_arr, 4, 10);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

    // update ERROR_STAT, containing the first received byte and the upper 4 bits of the last received byte.
    ERROR_STAT = (uint16_t)receive_frame.byte_arr[0];
    ERROR_STAT <<= 4;
    ERROR_STAT |= ((uint16_t)receive_frame.byte_arr[3] & 0xf0) >> 4;
    return __builtin_bswap32(receive_frame.data32);
}

/**
 * @brief Read the content of the register at the offset in the argument. This function attempts to read the register until the CRC is correct.
 * 
 * @param offset Offset of the register to read.
 * @param start_conversion_spi Set true to initiate conversion when the CS line goes high after the register read.
 * @return Register content read from the TMAG5170.
 */
uint16_t TMAG5170::readRegister(uint8_t offset, bool start_conversion_spi) {
    uint32_t sent_frame = ((uint32_t)offset | TMAG5170_READ_REG) << 24;
    if(start_conversion_spi) sent_frame |= START_CONVERSION;
    sent_frame |= generateCRC(sent_frame);

    uint32_t received_frame;
    do {
        received_frame = exchangeFrame(sent_frame);
    } while(checkCRC(received_frame) != 0);

    received_frame >>= 8;               // extract the middle two bytes for the register content
    received_frame &= 0xffff;
    uint16_t register_content = (uint16_t)received_frame;
    TMAG5170_registers[offset] = register_content;
    return register_content;
}

/**
 * @brief Write the register content stored in the array into the register at the offset provided in the argument. This function attempts to write the register until the returning CRC is correct.
 * 
 * @param offset Offset of the register to write.
 * @param start_conversion_spi Set true to initiate conversion when the CS line goes high after the register read.
 */
void TMAG5170::writeRegister(uint8_t offset, bool start_conversion_spi) {
    uint16_t register_content = TMAG5170_registers[offset];
    uint32_t sent_frame = ((uint32_t)offset << 24) | ((uint32_t)register_content << 8);
    if(start_conversion_spi) sent_frame |= START_CONVERSION;
    sent_frame |= generateCRC(sent_frame);

    uint32_t received_frame;
    do {
        received_frame = exchangeFrame(sent_frame);
    } while(checkCRC(received_frame) != 0);

    return;
}

/**
 * @brief Read the ERROR_STAT bits of the previously received frame.
 * 
 * @return uint16_t containing the ERROR_STAT bits.
 */
uint16_t TMAG5170::readERRORSTAT(void) {
    return ERROR_STAT;
}

/**
 * @brief Initialise TMAG5170, clear the CFG_REST bit and reads the version of the device.
 * 
 * @return Version of the device, A1 or A2.
 */
TMAG5170_version TMAG5170::init(void) {
    uint16_t AFE16;                     // reads the AFE_STATUS register twice, the MSB should be 0 by the second time.
    AFE16 = readRegister(AFE_STATUS);
    AFE16 = readRegister(AFE_STATUS);
    if(AFE16 & 0x8000) {
        version = TMAG5170_ERROR;
        return TMAG5170_ERROR;
    }

    uint16_t TEST16;
    TEST16 = readRegister(TEST_CONFIG);
    if((TEST16 & 0x0030) == (0x0 << 4)) {
        version = TMAG5170_A1;
        magnetic_coeff[0] = 50.0f / 32768.0f;
        magnetic_coeff[1] = 50.0f / 32768.0f;
        magnetic_coeff[2] = 50.0f / 32768.0f;
    } else if((TEST16 & 0x0030) == (0x1 << 4)) {
        version = TMAG5170_A2;
        magnetic_coeff[0] = 150.0f / 32768.0f;
        magnetic_coeff[1] = 150.0f / 32768.0f;
        magnetic_coeff[2] = 150.0f / 32768.0f;
    } else {
        version = TMAG5170_ERROR;
        magnetic_coeff[0] = 0.0f;
        magnetic_coeff[1] = 0.0f;
        magnetic_coeff[2] = 0.0f;
    }
    return version;
}

/**
 * @brief Set the operating mode of the TMAG5170.
 * 
 * @param operating_mode Use \ref OPERATING_MODE "macros" to specify the operating mode.
 */
void TMAG5170::setOperatingMode(uint16_t operating_mode) {
    TMAG5170_registers[DEVICE_CONFIG] &= ~OPERATING_MODE_MASK;
    TMAG5170_registers[DEVICE_CONFIG] |= operating_mode;
    writeRegister(DEVICE_CONFIG);

    return;
}

/**
 * @brief Set the number of averages to take for each conversion.
 * 
 * @param conversion_average Use \ref CONV_AVG "macros" to specify the number of averages.
 */
void TMAG5170::setConversionAverage(uint16_t conversion_average) {
    TMAG5170_registers[DEVICE_CONFIG] &= ~CONV_AVG_MASK;
    TMAG5170_registers[DEVICE_CONFIG] |= conversion_average;
    writeRegister(DEVICE_CONFIG);

    return;
}

/**
 * @brief Enable angle calculation.
 * 
 * @param angle_calculation_config Use \ref ANGLE_AXES "macros" to specify the two axes used for angle calculation.
 */
void TMAG5170::enableAngleCalculation(uint16_t angle_calculation_config) {
    TMAG5170_registers[SENSOR_CONFIG] &= ~ANGLE_EN_MASK;
    TMAG5170_registers[SENSOR_CONFIG] |= angle_calculation_config;
    writeRegister(SENSOR_CONFIG);

    return;
}

/**
 * @brief Enable conversion on a selection of the three magnetic axes.
 * 
 * @param x_enable Enable X axis.
 * @param y_enable Enable Y axis.
 * @param z_enable Enable Z axis.
 */
void TMAG5170::enableMagneticChannel(bool x_enable, bool y_enable, bool z_enable) {
    TMAG5170_registers[SENSOR_CONFIG] &= ~MAG_CH_EN_MASK;
    if(x_enable) {
        TMAG5170_registers[SENSOR_CONFIG] |= 0x0040;
    }
    if(y_enable) {
        TMAG5170_registers[SENSOR_CONFIG] |= 0x0080;
    }
    if(z_enable) {
        TMAG5170_registers[SENSOR_CONFIG] |= 0x0100;
    }
    writeRegister(SENSOR_CONFIG);

    return;
}

/**
 * @brief Set magnetic field measurement range on each axis.
 * 
 * @param x_range Use \ref MAG_RANGE "macros" to specify the range of the X axis.
 * @param y_range Use \ref MAG_RANGE "macros" to specify the range of the Y axis.
 * @param z_range Use \ref MAG_RANGE "macros" to specify the range of the Z axis.
 */
void TMAG5170::setMagneticRange(uint16_t x_range, uint16_t y_range, uint16_t z_range) {
    TMAG5170_registers[SENSOR_CONFIG] &= ~(X_RANGE_MASK | Y_RANGE_MASK | Z_RANGE_MASK);
    TMAG5170_registers[SENSOR_CONFIG] |= x_range | y_range | z_range;
    writeRegister(SENSOR_CONFIG);

    switch(x_range) {
    case X_RANGE_50mT:
        if(version == TMAG5170_A1) magnetic_coeff[0] = 50.0f / 32768.0f;
        if(version == TMAG5170_A2) magnetic_coeff[0] = 150.0f / 32768.0f;
        break;
    case X_RANGE_25mT:
        if(version == TMAG5170_A1) magnetic_coeff[0] = 25.0f / 32768.0f;
        if(version == TMAG5170_A2) magnetic_coeff[0] = 75.0f / 32768.0f;
        break;
    case X_RANGE_100mT:
        if(version == TMAG5170_A1) magnetic_coeff[0] = 100.0f / 32768.0f;
        if(version == TMAG5170_A2) magnetic_coeff[0] = 300.0f / 32768.0f;
        break;
    }

    switch(y_range) {
    case Y_RANGE_50mT:
        if(version == TMAG5170_A1) magnetic_coeff[1] = 50.0f / 32768.0f;
        if(version == TMAG5170_A2) magnetic_coeff[1] = 150.0f / 32768.0f;
        break;
    case Y_RANGE_25mT:
        if(version == TMAG5170_A1) magnetic_coeff[1] = 25.0f / 32768.0f;
        if(version == TMAG5170_A2) magnetic_coeff[1] = 75.0f / 32768.0f;
        break;
    case Y_RANGE_100mT:
        if(version == TMAG5170_A1) magnetic_coeff[1] = 100.0f / 32768.0f;
        if(version == TMAG5170_A2) magnetic_coeff[1] = 300.0f / 32768.0f;
        break;
    }

    switch(z_range) {
    case Z_RANGE_50mT:
        if(version == TMAG5170_A1) magnetic_coeff[2] = 50.0f / 32768.0f;
        if(version == TMAG5170_A2) magnetic_coeff[2] = 150.0f / 32768.0f;
        break;
    case Z_RANGE_25mT:
        if(version == TMAG5170_A1) magnetic_coeff[2] = 25.0f / 32768.0f;
        if(version == TMAG5170_A2) magnetic_coeff[2] = 75.0f / 32768.0f;
        break;
    case Z_RANGE_100mT:
        if(version == TMAG5170_A1) magnetic_coeff[2] = 100.0f / 32768.0f;
        if(version == TMAG5170_A2) magnetic_coeff[2] = 300.0f / 32768.0f;
        break;
    }

    return;
}

/**
 * @brief Enable ALERT output to notify the microcontroller when the conversion is finished.
 * 
 * @param enable Set true to enable ALERT output.
 */
void TMAG5170:: enableAlertOutput(bool enable) {
    TMAG5170_registers[ALERT_CONFIG] &= ~RSLT_ALRT_Asserted;
    if(enable) {
        TMAG5170_registers[ALERT_CONFIG] |= RSLT_ALRT_Asserted;
    }
    writeRegister(ALERT_CONFIG);

    return;
}

/**
 * @brief Read the raw 16-bit conversion result of the magnetic field on the X axis.
 * 
 * @param start_conversion_spi Set true to initiate conversion when the CS line goes high after the register read.
 * @return The raw 16-bit value in the X_CH_RESULT register.
 */
int16_t TMAG5170::readXRaw(bool start_conversion_spi) {
    return readRegister(X_CH_RESULT, start_conversion_spi);
}

/**
 * @brief Read the raw 16-bit conversion result of the magnetic field on the Y axis.
 * 
 * @param start_conversion_spi Set true to initiate conversion when the CS line goes high after the register read.
 * @return The raw 16-bit value in the Y_CH_RESULT register.
 */
int16_t TMAG5170::readYRaw(bool start_conversion_spi) {
    return readRegister(Y_CH_RESULT, start_conversion_spi);
}

/**
 * @brief Read the raw 16-bit conversion result of the magnetic field on the Z axis.
 * 
 * @param start_conversion_spi Set true to initiate conversion when the CS line goes high after the register read.
 * @return The raw 16-bit value in the Z_CH_RESULT register.
 */
int16_t TMAG5170::readZRaw(bool start_conversion_spi) {
    return readRegister(Z_CH_RESULT, start_conversion_spi);
}

/**
 * @brief Read the conversion result in mT on the X axis.
 * 
 * @param start_conversion_spi Set true to initiate conversion when the CS line goes high after the register read.
 * @return The magnetic field on the X axis in mT.
 */
float TMAG5170::readX(bool start_conversion_spi) {
    conversion_container container;
    container.unsigned16 = readRegister(X_CH_RESULT, start_conversion_spi);
    return container.signed16 * magnetic_coeff[0];
}

/**
 * @brief Read the conversion result in mT on the Y axis.
 * 
 * @param start_conversion_spi Set true to initiate conversion when the CS line goes high after the register read.
 * @return The magnetic field on the Y axis in mT.
 */
float TMAG5170::readY(bool start_conversion_spi) {
    conversion_container container;
    container.unsigned16 = readRegister(Y_CH_RESULT, start_conversion_spi);
    return container.signed16 * magnetic_coeff[1];
}

/**
 * @brief Read the conversion result in mT on the Z axis.
 * 
 * @param start_conversion_spi Set true to initiate conversion when the CS line goes high after the register read.
 * @return The magnetic field on the Z axis in mT.
 */
float TMAG5170::readZ(bool start_conversion_spi) {
    conversion_container container;
    container.unsigned16 = readRegister(Z_CH_RESULT, start_conversion_spi);
    return container.signed16 * magnetic_coeff[2];
}

/**
 * @brief Read the raw 16-bit data of the angle of the magnetic field.
 * 
 * @param start_conversion_spi Set true to initiate conversion when the CS line goes high after the register read.
 * @return The raw 16-bit data in the ANGLE_RESULT register.
 */
int16_t TMAG5170::readAngleRaw(bool start_conversion_spi) {
    return readRegister(ANGLE_RESULT, start_conversion_spi);
}

/**
 * @brief Read the angle of the magnetic field in degree.
 * 
 * @param start_conversion_spi Set true to initiate conversion when the CS line goes high after the register read.
 * @return The angle of the magnetic field in degree in float. 
 */
float TMAG5170::readAngle(bool start_conversion_spi) {
    conversion_container container;
    container.unsigned16 = readRegister(ANGLE_RESULT, start_conversion_spi);
    return container.signed16 / 16.0f;
}

/**
 * @brief Read the raw 16-bit magnitude of the magnetic field.
 * 
 * @param start_conversion_spi Set true to initiate conversion when the CS line goes high after the register read.
 * @return The raw 16-bit data in the MAGNITUDE_RESULT register. 
 */
int16_t TMAG5170::readMagnitudeRaw(bool start_conversion_spi) {
    return readRegister(MAGNITUDE_RESULT, start_conversion_spi);
}

// Reads the conversion result of the magnitude of the magnetic field.
// Returns the magnitude in mT.
// float TMAG5170::readMagnitude(bool start_conversion_spi) {
//     conversion_container container;
//     container.unsigned16 = readRegister(MAGNITUDE_RESULT, start_conversion_spi);
//     return container.signed16 * magnetic_coeff[2];
// }