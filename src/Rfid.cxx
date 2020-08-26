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
#include "config.h"

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
        Serial.println("RC522 self test failed");
        return false;
    }

    Serial.println("RC522 self test succeeded");

    pinMode(PIN_RFID_IRQ, INPUT);
    attachInterrupt(PIN_RFID_IRQ, rfidIsr, FALLING);

    mfrc522->PCD_Init();
    mfrc522->PCD_WriteRegister(mfrc522->ComIEnReg, 0xa0);

    return true;
}

void handleRfid(std::string uid) {
    if (jsonConfig->isRfidConfigured(uid)) {
        Audio::play(jsonConfig->albumForRfid(uid).c_str());
    } else {
        Serial.printf("scanned unmapped RFID %s\r\n", uid.c_str());
    }
}

void _rfidTask() {
    if (!setupMfrc522()) return;

    while (true) {
        MFRC522::Uid uid;

        mfrc522->PCD_WriteRegister(mfrc522->FIFODataReg, mfrc522->PICC_CMD_REQA);
        mfrc522->PCD_WriteRegister(mfrc522->CommandReg, mfrc522->PCD_Transceive);
        mfrc522->PCD_WriteRegister(mfrc522->BitFramingReg, 0x87);

        uint32_t value;
        if (xTaskNotifyWait(0x0, 0x0, &value, 100) == pdFALSE) continue;

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
            Serial.printf("RFID: failed to read UID: %i\r\n", (int)readSerialStatus);
        }

        if (piccHaltStatus != MFRC522::STATUS_OK)
            Serial.printf("RFID: failed to send HALT to PICC: %i\r\n", (int)readSerialStatus);

        mfrc522->PCD_WriteRegister(mfrc522->ComIrqReg, 0x7f);
        xTaskNotifyWait(0x0, 0x0, &value, 0);
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
