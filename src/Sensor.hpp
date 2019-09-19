#ifndef SENSOR_HPP
#define SENSOR_HPP

#include <Windows.h>

#ifdef __cplusplus
#include <iostream>
#include <string>
class Sensor
{
public:
    Sensor();
    ~Sensor();
    void SerialInit(char *port);
    static void SerialReadThread(void* myInstant);
    void SerialRead();

    bool in_use = false;
    int  direction = 0;
    int  speed = 0;

private:
    void ReadStateMachine(int b);

    HANDLE _handle = 0;
    HANDLE _thread = 0;
    DWORD _id = 0;
    bool _run = false;
    int _s = 0;
    int _v = 0;
    int _d = 0;
    int _d_last = 0;
};
#else
typedef struct Sensor Sensor;
#endif

#ifdef __cplusplus
extern "C"
{
#endif
    extern Sensor* G_sensor;
    extern int sensor_in_use(Sensor* s);
    extern int sensor_direction(Sensor* s);
    extern int sensor_speed(Sensor* s);
#ifdef __cplusplus
}
#endif

#endif // SENSOR_HPP

/* EOF */
