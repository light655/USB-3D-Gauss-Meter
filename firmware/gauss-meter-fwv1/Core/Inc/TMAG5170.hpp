/**
 * @file TMAG5170.hpp
 * @author Chen, Liang-Yu
 * @brief TMAG5170 library for STM32
 * @version 0.1
 * @date 2026-01-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef TMAG5170_H
#define TMAG5170_H

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_spi.h"

// -------------------- Register offset --------------------
/** @brief Use READ_REG to set the MSB to read register */
#define TMAG5170_READ_REG 0x80
/** @defgroup REG_OFFSET Register offset 
 * Register offset to go into the 1st byte of the SPI frame.
 * @{
 */
#define DEVICE_CONFIG 0x00      ///< @brief Offset of DEVICE_CONFIG register
#define SENSOR_CONFIG 0x01      ///< @brief Offset of SENSOR_CONFIG register
#define SYSTEM_CONFIG 0x02      ///< @brief Offset of SYSTEM_CONFIG register
#define ALERT_CONFIG 0x03       ///< @brief Offset of ALERT_CONFIG register
#define X_THRX_CONFIG 0x04      ///< @brief Offset of X_THRX_CONFIG register
#define Y_THRX_CONFIG 0x05      ///< @brief Offset of Y_THRX_CONFIG register
#define Z_THRX_CONFIG 0x06      ///< @brief Offset of Z_THRX_CONFIG register
#define T_THRX_CONFIG 0x07      ///< @brief Offset of T_THRX_CONFIG register
#define CONV_STATUS 0x08        ///< @brief Offset of CONV_STATUS register
#define X_CH_RESULT 0x09        ///< @brief Offset of X_CH_RESULT register
#define Y_CH_RESULT 0x0A        ///< @brief Offset of Y_CH_RESULT register
#define Z_CH_RESULT 0x0B        ///< @brief Offset of Z_CH_RESULT register
#define TEMP_RESULT 0x0C        ///< @brief Offset of TEMP_RESULT register
#define AFE_STATUS 0x0D         ///< @brief Offset of AFE_STATUS register
#define SYS_STATUS 0x0E         ///< @brief Offset of SYS_STATUS register
#define TEST_CONFIG 0x0F        ///< @brief Offset of TEST_CONFIG register
#define OSC_MONITOR 0x10        ///< @brief Offset of OSC_MONITOR register
#define MAG_GAIN_CONFIG 0x11    ///< @brief Offset of MAG_GAIN_CONFIG register
#define ANGLE_RESULT 0x13       ///< @brief Offset of ANGLE_RESULT register
#define MAGNITUDE_RESULT 0x14   ///< @brief Offset of MAGNITUDE_RESULT register
/**
 * @} 
 */

// Settings in the registers are expressed as 16bit values
//------------------DEVICE_CONFIG------------------------
#define CONV_AVG_MASK 0x7000
#define MAG_TEMPCO_MASK 0x300
#define OPERATING_MODE_MASK 0x70

/** @defgroup CONV_AVG Number of averages per conversion
 * @{
 */
#define CONV_AVG_1x 0x0000      ///< @brief No averaging, single conversion
#define CONV_AVG_2x 0x1000      ///< @brief Average 2 conversions
#define CONV_AVG_4x 0x2000      ///< @brief Average 4 conversions
#define CONV_AVG_8x 0x3000      ///< @brief Average 8 conversions
#define CONV_AVG_16x 0x4000     ///< @brief Average 16 conversions
#define CONV_AVG_32x 0x5000     ///< @brief Average 32 conversions
/** @} */

#define MAG_TEMPCO_0pd 0x000
#define MAG_TEMPCO_012pd 0x100
#define MAG_TEMPCO_003pd 0x200
#define MAG_TEMPCO_02pd 0x300

/** @defgroup OPERATING_MODE Operating mode of TMAG5170
 * @{
 */
#define OPERATING_MODE_ConfigurationMode 0x00   ///< @brief Configuration mode
#define OPERATING_MODE_StandbyMode 0x10         ///< @brief Standby mode
#define OPERATING_MODE_ActiveMeasureMode 0x20   ///< @brief Active measure mode
#define OPERATING_MODE_ActiveTriggerMode 0x30   ///< @brief Active trigger mode
#define OPERATING_MODE_WakeupAndSleepMode 0x40  ///< @brief Wakeup and sleep mode
#define OPERATING_MODE_SleepMode 0x50           ///< @brief Sleep mode
#define OPERATING_MODE_DeepsleepMode 0x60       ///< @brief Deep sleep mode
/**
 * @} 
 */

#define T_CH_EN_TempChannelDisabled 0x0
#define T_CH_EN_TempChannelEnabled 0x8

#define T_RATE_SameAsOtherSensors 0x0
#define T_RATE_OncePerConversionSet 0x4

#define T_HLT_EN_TempLimitCheckOff 0x0
#define T_HLT_EN_TempLimitCheckOn 0x2

//------------------SENSOR_CONFIG------------------------
#define ANGLE_EN_MASK 0xC000
#define SLEEPTIME_MASK 0x3C00
#define MAG_CH_EN_MASK 0x03C0
#define Z_RANGE_MASK 0x0030
#define Y_RANGE_MASK 0x000C
#define X_RANGE_MASK 0x0003

/** @defgroup ANGLE_AXES Angle calculation axes configuration
 * @{
 */
#define ANGLE_EN_NoAngleCalculation 0x0         ///< @brief Angle calculation disabled
#define ANGLE_EN_X_Y 0x4000                     ///< @brief Angle calculation using X and Y axes
#define ANGLE_EN_Y_Z 0x8000                     ///< @brief Angle calculation using Y and Z axes
#define ANGLE_EN_Z_X 0xC000                     ///< @brief Angle calculation using Z and X axes
/** @} */

/** @defgroup SLEEPTIME Sleeptime
 * Time spent in sleep between conversions in active measure mode.
 * @{
 */
#define SLEEPTIME_1ms 0x0           ///< @brief Sleep time 1ms
#define SLEEPTIME_5ms 0x400         ///< @brief Sleep time 5ms
#define SLEEPTIME_10ms 0x800        ///< @brief Sleep time 10ms
#define SLEEPTIME_15ms 0xC00        ///< @brief Sleep time 15ms
#define SLEEPTIME_20ms 0x1000       ///< @brief Sleep time 20ms
#define SLEEPTIME_30ms 0x1400       ///< @brief Sleep time 30ms
#define SLEEPTIME_50ms 0x1800       ///< @brief Sleep time 50ms
#define SLEEPTIME_100ms 0x1C00      ///< @brief Sleep time 100ms
#define SLEEPTIME_500ms 0x2000      ///< @brief Sleep time 500ms
#define SLEEPTIME_1000ms 0x2400     ///< @brief Sleep time 1000ms
/** @} */

#define MAG_CH_EN_OFF 0x0
#define MAG_CH_EN_Xenabled 0x40
#define MAG_CH_EN_Yenabled 0x80
#define MAG_CH_EN_XYenabled 0xC0
#define MAG_CH_EN_Zenabled 0x100
#define MAG_CH_EN_ZXenabled 0x140
#define MAG_CH_EN_YZenabled 0x180
#define MAG_CH_EN_XYZenabled 0x1C0
#define MAG_CH_EN_XYXenabled 0x200
#define MAG_CH_EN_YXYenabled 0x240
#define MAG_CH_EN_YZYenabled 0x280
#define MAG_CH_EN_ZYZenabled 0x2C0
#define MAG_CH_EN_ZXZenabled 0x300
#define MAG_CH_EN_XZXenabled 0x340
#define MAG_CH_EN_XYZYXenabled 0x380
#define MAG_CH_EN_XYZZYXenabled 0x3C0

/** @defgroup MAG_RANGE Magnetic field range for conversion 
 * @{
*/
#define Z_RANGE_50mT 0x0       ///< @brief Z-axis magnetic range: ±50 mT (A1 variant)
#define Z_RANGE_25mT 0x10      ///< @brief Z-axis magnetic range: ±25 mT (A1 variant)
#define Z_RANGE_100mT 0x20     ///< @brief Z-axis magnetic range: ±100 mT (A1 variant)

#define Y_RANGE_50mT 0x0       ///< @brief Y-axis magnetic range: ±50 mT (A1 variant)
#define Y_RANGE_25mT 0x4       ///< @brief Y-axis magnetic range: ±25 mT (A1 variant)
#define Y_RANGE_100mT 0x8      ///< @brief Y-axis magnetic range: ±100 mT (A1 variant)

#define X_RANGE_50mT 0x0       ///< @brief X-axis magnetic range: ±50 mT (A1 variant)
#define X_RANGE_25mT 0x1       ///< @brief X-axis magnetic range: ±25 mT (A1 variant)
#define X_RANGE_100mT 0x2      ///< @brief X-axis magnetic range: ±100 mT (A1 variant)
//------------------A2 variant---------------------------
#define Z_RANGE_150mT 0x0      ///< @brief Z-axis magnetic range: ±150 mT (A2 variant)
#define Z_RANGE_75mT 0x10      ///< @brief Z-axis magnetic range: ±75 mT (A2 variant)
#define Z_RANGE_300mT 0x20     ///< @brief Z-axis magnetic range: ±300 mT (A2 variant)

#define Y_RANGE_150mT 0x0      ///< @brief Y-axis magnetic range: ±150 mT (A2 variant)
#define Y_RANGE_75mT 0x4       ///< @brief Y-axis magnetic range: ±75 mT (A2 variant)
#define Y_RANGE_300mT 0x8      ///< @brief Y-axis magnetic range: ±300 mT (A2 variant)

#define X_RANGE_150mT 0x0      ///< @brief X-axis magnetic range: ±150 mT (A2 variant)
#define X_RANGE_75mT 0x1       ///< @brief X-axis magnetic range: ±75 mT (A2 variant)
#define X_RANGE_300mT 0x2      ///< @brief X-axis magnetic range: ±300 mT (A2 variant)
/** @} */

//------------------SYSTEM_CONFIG------------------------
#define DIAG_SEL_MASK 0x3000
#define TRIGGER_MODE_MASK 0x0600
#define DATA_TYPE_MASK 0x01C0

#define DIAG_SEL_AllDataPath 0x0
#define DIAG_SEL_EnabledDataPath 0x1000
#define DIAG_SEL_AllDataPathInSequence 0x2000
#define DIAG_SEL_EnabledDataPathInSequence 0x3000

#define TRIGGER_MODE_SPI 0x0
#define TRIGGER_MODE_nCS 0x200
#define TRIGGER_MODE_nALERT 0x400

#define DATA_TYPE_32bit 0x0
#define DATA_TYPE_12bit_XY 0x40
#define DATA_TYPE_12bit_XZ 0x80
#define DATA_TYPE_12bit_ZY 0xC0
#define DATA_TYPE_12bit_XT 0x100
#define DATA_TYPE_12bit_YT 0x140
#define DATA_TYPE_12bit_ZT 0x180
#define DATA_TYPE_12bit_AM 0x1C0

#define DIAG_EN_AFEdiagnosticsDisabled 0x0
#define DIAG_EN_AFEdiagnosticsEnabled 0x20

#define Z_HLT_EN_ZaxisLimitCheckoff 0x0
#define Z_HLT_EN_ZaxisLimitCheckon 0x4

#define Y_HLT_EN_YaxisLimitCheckoff 0x0
#define Y_HLT_EN_YaxisLimitCheckon 0x2

#define X_HLT_EN_XaxisLimitCheckoff 0x0
#define X_HLT_EN_XaxisLimitCheckon 0x1

//------------------ALERT_CONFIG------------------------
#define ALERT_LATCH_SourcesNotLatched 0x0
#define ALERT_LATCH_SourcesLatched 0x2000

#define ALERT_MODE_InterruptMode 0x0
#define ALERT_MODE_SwitchMode 0x1000

#define STATUS_ALRT_NotAsserted 0x0
#define STATUS_ALRT_Asserted 0x800

#define RSLT_ALRT_NotAsserted 0x0
#define RSLT_ALRT_Asserted 0x100

#define THRX_COUNT_1_ConversionResult 0x0
#define THRX_COUNT_2_ConversionResult 0x10
#define THRX_COUNT_3_ConversionResult 0x20
#define THRX_COUNT_4_ConversionResult 0x30

#define T_THRX_ALRT_NotAsserted 0x0
#define T_THRX_ALRT_Asserted 0x8

#define Z_THRX_ALRT_NotAsserted 0x0
#define Z_THRX_ALRT_Asserted 0x4

#define Y_THRX_ALRT_NotAsserted 0x0
#define Y_THRX_ALRT_Asserted 0x2

#define X_THRX_ALRT_NotAsserted 0x0
#define X_THRX_ALRT_Asserted 0x1

// ------------------ X_THRX_CONFIG ------------------
#define X_HI_THRESHOLD_MASK 0xff00
#define X_LO_THRESHOLD_MASK 0x00ff

// ------------------ Y_THRX_CONFIG ------------------
#define Y_HI_THRESHOLD_MASK 0xff00
#define Y_LO_THRESHOLD_MASK 0x00ff

// ------------------ Z_THRX_CONFIG ------------------
#define Z_HI_THRESHOLD_MASK 0xff00
#define Z_LO_THRESHOLD_MASK 0x00ff

// ------------------ AFE_STATUS ------------------
#define CFG_RESET_MASK 0x8000
#define SENS_STAT_MASK 0x1000
#define TEMP_STAT_MASK 0x0800
#define ZHS_STAT_MASK 0x0400
#define YHS_STAT_MASK 0x0200
#define XHS_STAT_MASK 0x0100
#define TRIM_STAT_MASK 0x0002
#define LDO_STAT_MASK 0x0001

// ------------------ SYS_STATUS ------------------
#define ALRT_LVL_MASK 0x8000
#define ALRT_DRV_MASK 0x4000
#define SDO_DRV_MASK 0x2000
#define CRC_STAT_MASK 0x1000
#define FRAME_STAT_MASK 0x0800
#define OPERATING_STAT_MASK 0x0700
#define VCC_OV_MASK 0x0020
#define VCC_UV_MASK 0x0010
#define TEMP_THX_MASK 0x0008
#define ZCH_THX_MASK 0x0004
#define YCH_THX_MASK 0x0002
#define XCH_THX_MASK 0x0001

//------------------TEST_CONFIG------------------------
#define VER_MASK 0x0030

#define CRC_DIS_CRCenabled 0x0
#define CRC_DIS_CRCdisabled 0x4

#define OSC_CNT_CTL_ResetCounter 0x0
#define OSC_CNT_CTL_StartCounterHFOSC 0x1
#define OSC_CNT_CTL_StartCounterLFOSC 0x2
#define OSC_CNT_CTL_StopCounter 0x3

//------------------MAG_GAIN_CONFIG------------------------
#define GAIN_SELECTION_MASK 0xC000
#define GAIN_VALUE_MASK 0x03FF

#define GAIN_SELECTION_NoAxis 0x0
#define GAIN_SELECTION_Xselected 0x4000
#define GAIN_SELECTION_Yselected 0x8000
#define GAIN_SELECTION_Zselected 0xC000

// ------------------ MAG_OFFSET_CONFIG -----------------
#define OFFSET_SELECTION_MASK 0xC000
#define OFFSET_VALUE1_MASK 0x3F10
#define OFFSET_VALUE2_MASK 0x007F

#define OFFSET_SELECTION_NoSelected 0x0000
#define OFFSET_SELECTION_Use1 0x4000
#define OFFSET_SELECTION_Use2 0x8000
#define OFFSET_SELECTION_Both 0xC000

// ------------------- ERROR_STAT -------------------
#define PREV_CRC_STAT_MASK 0x0800
#define ERROR_STAT_CFG_RESET_MASK 0x0400
#define ALRT_AFE_MASK 0x0200
#define ALRT_SYS_MASK 0x0100
#define X_CURRENT_MASK 0x0080
#define Y_CURRENT_MASK 0x0040
#define Z_CURRENT_MASK 0x0020
#define T_CURRENT_MASK 0x0010
#define ERROR_STAT_MASK 0x0008
#define COUNT_MASK 0x0007
#define ERROR_STAT_DATA_TYPE_MASK 0x0007

// ------------------- SPI command -------------------
// SPI command to go into the last byte of the SPI frame
#define START_CONVERSION 0x10

#ifdef __cplusplus
extern "C" {
#endif
    extern SPI_HandleTypeDef hspi1;
#ifdef __cplusplus
}
#endif

/**
 * @brief Enumeration for TMAG5170 version: A1 or A2.
 * 
 */
enum TMAG5170_version {
    TMAG5170_A1 = 0x0, TMAG5170_A2 = 0x1, TMAG5170_ERROR = 0x3
};

/**
 * @brief Class for TMAG5170.
 * 
 */
class TMAG5170 {
    private:
        typedef union {
            uint8_t byte_arr[4];
            uint32_t data32;
        } TMAG5170_SPI_frame;

        typedef union {
            uint16_t unsigned16;
            int16_t signed16;
        } conversion_container;

        TMAG5170_version version;
        uint16_t ERROR_STAT;

        uint16_t TMAG5170_registers[21] {
            0x0000, 0x0000, 0x0000, 0x0000,
            0x7D83, 0x7D83, 0x7D83, 0x6732,
            0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x8000, 0x0000, 0x8000,
            0x0000, 0x0000, 0x0000, 0x0000,
            0x0000
        };
        float magnetic_coeff[3];

    public:
        TMAG5170(void);

        uint32_t generateCRC(uint32_t data);
        int checkCRC(uint32_t received_frame);
        uint32_t exchangeFrame(uint32_t frame);
        uint16_t readRegister(uint8_t offset, bool start_conversion_spi = false);
        void writeRegister(uint8_t offset, bool start_conversion_spi = false);
        uint16_t readERRORSTAT(void);

        TMAG5170_version init(void);
        void setOperatingMode(uint16_t operating_mode);
        void setConversionAverage(uint16_t conversion_average);
        void enableAngleCalculation(uint16_t angle_calculation_config);
        void enableMagneticChannel(bool x_enable, bool y_enable, bool z_enable);
        void setMagneticRange(uint16_t x_range, uint16_t y_range, uint16_t z_range);
        void enableAlertOutput(bool enable);

        int16_t readXRaw(bool start_conversion_spi = false);
        int16_t readYRaw(bool start_conversion_spi = false);
        int16_t readZRaw(bool start_conversion_spi = false);
        float readX(bool start_conversion_spi = false);
        float readY(bool start_conversion_spi = false);
        float readZ(bool start_conversion_spi = false);

        int16_t readAngleRaw(bool start_conversion_spi = false);
        float readAngle(bool start_conversion_spi = false);
        int16_t readMagnitudeRaw(bool start_conversion_spi = false);
        // float readMagnitude(bool start_conversion_spi = false);
};

#endif
