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

#include "Lock.hxx"
#include "config.h"

namespace {

DRAM_ATTR QueueHandle_t rfidInterruptQueue;
SPIClass* spi;
SemaphoreHandle_t spiMutex;

extern "C" IRAM_ATTR void rfidIsr() {
    uint32_t val = 1;

    xQueueSendFromISR(rfidInterruptQueue, &val, NULL);
}

void _rfidTask() {
    MFRC522 mfrc522(*spi);

    {
        Lock lock(spiMutex);

        mfrc522.PCD_Init(PIN_RFID_CS, MFRC522::UNUSED_PIN);
    }

    delay(10);

    bool selfTestSucceeded;
    {
        Lock lock(spiMutex);

        selfTestSucceeded = mfrc522.PCD_PerformSelfTest();
    }

    if (!selfTestSucceeded) {
        Serial.println("RC522 self test failed");
        return;
    }

    Serial.println("RC522 self test succeeded");

    rfidInterruptQueue = xQueueCreate(1, 4);
    pinMode(PIN_RFID_IRQ, INPUT);
    attachInterrupt(PIN_RFID_IRQ, rfidIsr, FALLING);

    {
        Lock lock(spiMutex);

        mfrc522.PCD_Init();
        mfrc522.PCD_WriteRegister(mfrc522.ComIEnReg, 0xa0);
    }

    while (true) {
        {
            Lock lock(spiMutex);

            mfrc522.PCD_WriteRegister(mfrc522.FIFODataReg, mfrc522.PICC_CMD_REQA);
            mfrc522.PCD_WriteRegister(mfrc522.CommandReg, mfrc522.PCD_Transceive);
            mfrc522.PCD_WriteRegister(mfrc522.BitFramingReg, 0x87);
        }

        uint32_t value;
        if (xQueueReceive(rfidInterruptQueue, &value, 100) != pdTRUE) continue;

        bool readSerialSucceeded;

        {
            Lock lock(spiMutex);

            mfrc522.PCD_WriteRegister(mfrc522.ComIrqReg, 0x7f);
            readSerialSucceeded = mfrc522.PICC_ReadCardSerial();
            mfrc522.PICC_HaltA();
        }

        if (readSerialSucceeded) {
            std::stringstream sstream;

            for (uint8_t i = 0; i < mfrc522.uid.size; i++) {
                sstream << std::setw(2) << std::setfill('0') << std::hex << (int)mfrc522.uid.uidByte[i] << " ";
            }

            Serial.printf("RFID: %s\r\n", sstream.str().c_str());
        }
    }
}

void rfidTask(void*) {
    _rfidTask();
    vTaskDelete(NULL);
}

}  // namespace

void Rfid::initialize(SPIClass& _spi, void* _spiMutex) {
    spi = &_spi;
    spiMutex = _spiMutex;
}

void Rfid::start() {
    TaskHandle_t rfidTaskHandle;
    xTaskCreatePinnedToCore(rfidTask, "rfid", STACK_SIZE_RFID, NULL, TASK_PRIORITY_RFID, &rfidTaskHandle, SERVICE_CORE);
}
