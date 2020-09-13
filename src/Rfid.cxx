#include "Rfid.hxx"

// clang-format off
#include <freertos/FreeRTOS.h>
// clang-format on

#include <SPI.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <iomanip>
#include <sstream>

#include "Audio.hxx"
#include "JsonConfig.hxx"
#include "MFRC522/MFRC522.h"
#include "Log.hxx"
#include "config.h"

#define TAG "rfid"

namespace {

DRAM_ATTR TaskHandle_t rfidTaskHandle;

SemaphoreHandle_t spiMutex;
SPIClass* spi;
MFRC522* mfrc522;

const JsonConfig* jsonConfig;

extern "C" IRAM_ATTR void rfidIsr() { xTaskNotifyFromISR(rfidTaskHandle, 0, eNoAction, NULL); }

bool setupMfrc522() {
    mfrc522 = new MFRC522(*spi, spiMutex);

    mfrc522->PCD_Init(PIN_RFID_CS, MFRC522::UNUSED_PIN);

    delay(10);

    bool selfTestSucceeded;
    selfTestSucceeded = mfrc522->PCD_PerformSelfTest();

    if (!selfTestSucceeded) {
        LOG_ERROR(TAG, "RC522 self test failed");
        return false;
    }

    LOG_INFO(TAG, "RC522 self test succeeded");

    mfrc522->PCD_Init();

    delay(10);

    LOG_INFO(TAG, "firmware version: 0x%02x", mfrc522->PCD_ReadRegister(MFRC522::VersionReg));

    pinMode(PIN_RFID_IRQ, INPUT);
    attachInterrupt(PIN_RFID_IRQ, rfidIsr, FALLING);
    mfrc522->PCD_WriteRegister(MFRC522::ComIEnReg, 0xa0);
    mfrc522->PCD_WriteRegister(MFRC522::DivIEnReg, 0x00);
    mfrc522->PCD_SetAntennaGain(0x70);

    return true;
}

void handleRfid(std::string uid) {
    if (jsonConfig->isRfidConfigured(uid)) {
        Audio::play(jsonConfig->albumForRfid(uid).c_str());
    } else {
        Audio::signalError();
        LOG_INFO(TAG, "scanned unmapped RFID %s", uid.c_str());
    }
}

void _rfidTask() {
    if (!setupMfrc522()) return;

    uint8_t referenceVersion = mfrc522->PCD_ReadRegister(MFRC522::VersionReg);

    while (true) {
        MFRC522::Uid uid;
        uint32_t value;

        uint8_t error = mfrc522->PCD_ReadRegister(MFRC522::ErrorReg) & 0xdf;
        if (error) LOG_ERROR(TAG, "MFRC522 error %i", (int)error);

        uint8_t version = mfrc522->PCD_ReadRegister(MFRC522::VersionReg);
        if (referenceVersion != version)
            LOG_ERROR(TAG, "failed to read back version: %d != %d; communication with RFID may be down",
                      (int)referenceVersion, version);

        mfrc522->PCD_WriteRegister(MFRC522::CommandReg, MFRC522::PCD_Idle);
        mfrc522->PCD_WriteRegister(MFRC522::TxModeReg, 0x00);
        mfrc522->PCD_WriteRegister(MFRC522::RxModeReg, 0x00);
        mfrc522->PCD_WriteRegister(MFRC522::ModWidthReg, 0x26);
        mfrc522->PCD_WriteRegister(MFRC522::FIFOLevelReg, 0x80);
        mfrc522->PCD_WriteRegister(MFRC522::FIFODataReg, MFRC522::PICC_CMD_REQA);
        mfrc522->PCD_WriteRegister(MFRC522::CommandReg, MFRC522::PCD_Transceive);
        mfrc522->PCD_WriteRegister(MFRC522::ComIrqReg, 0x7f);

        xTaskNotifyWait(0x0, 0x0, &value, 0);
        mfrc522->PCD_WriteRegister(MFRC522::BitFramingReg, 0x87);

        if (xTaskNotifyWait(0x0, 0x0, &value, 100) == pdFALSE) continue;

        LOG_INFO(TAG, "RFID RX interrupt: 0x%02x", mfrc522->PCD_ReadRegister(MFRC522::ComIrqReg));

        MFRC522::StatusCode readSerialStatus = mfrc522->PICC_Select(&uid);
        MFRC522::StatusCode piccHaltStatus = mfrc522->PICC_HaltA();

        if (readSerialStatus == MFRC522::STATUS_OK) {
            std::stringstream sstream;

            for (uint8_t i = 0; i < uid.size; i++) {
                sstream << std::setw(2) << std::setfill('0') << std::hex << (int)uid.uidByte[i];
                if (i != uid.size - 1) sstream << ":";
            }

            handleRfid(sstream.str());
        } else {
            LOG_INFO(TAG, "RFID: failed to read UID: %i", (int)readSerialStatus);
        }

        if (piccHaltStatus != MFRC522::STATUS_OK)
            LOG_ERROR(TAG, "RFID: failed to send HALT to PICC: %i", (int)readSerialStatus);
    }
}

void rfidTask(void*) {
    _rfidTask();
    vTaskDelete(NULL);
}

}  // namespace

void Rfid::initialize(SPIClass& _spi, void* _spiMutex, const JsonConfig& config) {
    spi = &_spi;
    spiMutex = _spiMutex;
    jsonConfig = &config;
}

void Rfid::start() {
    xTaskCreatePinnedToCore(rfidTask, "rfid", STACK_SIZE_RFID, NULL, TASK_PRIORITY_RFID, &rfidTaskHandle, SERVICE_CORE);
}
