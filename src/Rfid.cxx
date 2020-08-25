#include "Rfid.hxx"

// clang-format off
#include <freertos/FreeRTOS.h>
// clang-format on

#include <MFRC522.h>
#include <SPI.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <iomanip>
#include <sstream>

#include "Audio.hxx"
#include "JsonConfig.hxx"
#include "Lock.hxx"
#include "config.h"

namespace {

DRAM_ATTR QueueHandle_t rfidInterruptQueue;

SemaphoreHandle_t spiMutex;
SPIClass* spi;
MFRC522* mfrc522;

const JsonConfig* jsonConfig;

extern "C" IRAM_ATTR void rfidIsr() {
    uint32_t val = 1;

    xQueueSendFromISR(rfidInterruptQueue, &val, NULL);
}

bool setupMfrc522() {
    mfrc522 = new MFRC522(*spi);

    {
        Lock lock(spiMutex);

        mfrc522->PCD_Init(PIN_RFID_CS, MFRC522::UNUSED_PIN);
    }

    delay(10);

    bool selfTestSucceeded;
    {
        Lock lock(spiMutex);

        selfTestSucceeded = mfrc522->PCD_PerformSelfTest();
    }

    if (!selfTestSucceeded) {
        Serial.println("RC522 self test failed");
        return false;
    }

    Serial.println("RC522 self test succeeded");

    rfidInterruptQueue = xQueueCreate(1, 4);
    pinMode(PIN_RFID_IRQ, INPUT);
    attachInterrupt(PIN_RFID_IRQ, rfidIsr, FALLING);

    {
        Lock lock(spiMutex);

        mfrc522->PCD_Init();
        mfrc522->PCD_WriteRegister(mfrc522->ComIEnReg, 0xa0);
    }

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
        {
            Lock lock(spiMutex);

            mfrc522->PCD_WriteRegister(mfrc522->FIFODataReg, mfrc522->PICC_CMD_REQA);
            mfrc522->PCD_WriteRegister(mfrc522->CommandReg, mfrc522->PCD_Transceive);
            mfrc522->PCD_WriteRegister(mfrc522->BitFramingReg, 0x87);
        }

        uint32_t value;
        if (xQueueReceive(rfidInterruptQueue, &value, 100) != pdTRUE) continue;

        bool readSerialSucceeded;

        {
            Lock lock(spiMutex);

            mfrc522->PCD_WriteRegister(mfrc522->ComIrqReg, 0x7f);
            readSerialSucceeded = mfrc522->PICC_ReadCardSerial();
            mfrc522->PICC_HaltA();
        }

        if (readSerialSucceeded) {
            std::stringstream sstream;

            for (uint8_t i = 0; i < mfrc522->uid.size; i++) {
                sstream << std::setw(2) << std::setfill('0') << std::hex << (int)mfrc522->uid.uidByte[i];
                if (i != mfrc522->uid.size - 1) sstream << ":";
            }

            handleRfid(sstream.str());
        }
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
    TaskHandle_t rfidTaskHandle;
    xTaskCreatePinnedToCore(rfidTask, "rfid", STACK_SIZE_RFID, NULL, TASK_PRIORITY_RFID, &rfidTaskHandle, SERVICE_CORE);
}
