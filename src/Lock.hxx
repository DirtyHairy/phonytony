#ifndef LOCK_HXX
#define LOCK_HXX

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class Lock {
   public:
    Lock(SemaphoreHandle_t mutex);

    ~Lock();

   private:
    SemaphoreHandle_t mutex;
};

#endif  // LOCK_HXX
