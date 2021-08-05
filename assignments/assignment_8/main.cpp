// This example shows how to start threads and to share data between them with a struct.
// The struct is protected with a mutex to prevent simultaneous access.

#include <cstdint>
#include <string.h>

#include "mbed.h"

// Struct for sharing data between threads
typedef struct Data {
    Mutex mutex;
    uint16_t SecCount = 0;
    uint16_t minCount = 0;
} Data;

void secCountThread(Data *data)
{
    while (true) {
    data->mutex.lock();
    data->SecCount += 1;
    data->mutex.unlock();
     ThisThread::sleep_for(1s);
    }
}

void minCountThread(Data *data)
{
    while (true) {
        data->mutex.lock();
        data->minCount += 1;
        data->mutex.unlock();
        ThisThread::sleep_for(60s);
    }
}

void printCount(Data *data)
{
    while (true) {
        data->mutex.lock();
        printf("Minutes: %d\n", data->minCount);
        printf("Seconds: %d\n", data->SecCount);
        data->mutex.unlock();
        ThisThread::sleep_for(500ms);
    }
}


int main()
{   
    Data data;
    Thread t1(osPriorityHigh), t2(osPriorityHigh), t3;

    while (true) {
        //t1.set_priority(osPriorityNormal);
        //t2.set_priority(osPriorityNormal);
        t1.start(callback(secCountThread, &data));
        t2.start(callback(minCountThread, &data));
        t3.start(callback(printCount, &data));
        //test_thread((void *)"Th 1");
        ThisThread::sleep_for(1s);
    }
}
