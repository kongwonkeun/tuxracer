/**/

#include "Sensor.hpp"

#define MY_PIPE_FOO TEXT("\\\\.\\pipe\\foo")
#define MY_PIPE_SCORE TEXT("\\\\.\\pipe\\RDTLauncherPipe")

Sensor::Sensor()
{
    //
}

Sensor::~Sensor()
{
    _run = false;
    if (_thread) CloseHandle(_thread);
    if (_handle) CloseHandle(_handle);
}

void Sensor::SerialInit(char* port)
{
    //---- kong ----
    _handle = CreateFile(
        MY_PIPE_FOO,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );
    if (_handle == INVALID_HANDLE_VALUE) {
        throw("error: could not open pipe");
    }
    else {
        //
    }

    _score = CreateFile(
        MY_PIPE_SCORE,
        GENERIC_WRITE,
        FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );
    if (_score == INVALID_HANDLE_VALUE) {
        //throw("error: could not open pipe");
    }
    else {
        //
    }
    //----

    /*
    std::cout << port << std::endl;
    std::string p;
    if (port[4] == NULL) p = std::string(port); // COM1 ~ COM9
    else p = "\\\\.\\" + std::string(port); // COM10 ~

    //---- kong ----
    std::wstring wp = std::wstring(p.begin(), p.end());
    //----

    _handle = CreateFile(wp.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (_handle == INVALID_HANDLE_VALUE) {
        throw("error: could not open com port");
    }
    else {
        COMMTIMEOUTS timeout = { 100, 0, 0, 0, 0 };
        DCB dcb;
        if (!SetCommTimeouts(_handle, &timeout)) {
            Sensor::~Sensor();
            throw("error: could not set com port timeout");
        }
        memset(&dcb, 0, sizeof(dcb));
        dcb.DCBlength = sizeof(dcb);
        dcb.BaudRate = 115200; //---- kong ----
        dcb.fBinary = 1;
        dcb.fDtrControl = DTR_CONTROL_ENABLE;
        dcb.fRtsControl = RTS_CONTROL_ENABLE;
        dcb.Parity = NOPARITY;
        dcb.StopBits = ONE5STOPBITS;
        dcb.ByteSize = 8;
        if (!SetCommState(_handle, &dcb)) {
            Sensor::~Sensor();
            throw("error: could not set com port parameters");
        }
    }
    */
    in_use = true;
    _run = true;
    _thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(Sensor::SerialReadThread), this, NULL, &_id);
}

void Sensor::SendScore(int score)
{
    char b[16];
    DWORD count;

    std::string s = "S" + std::to_string(score);
    strcpy_s(b, s.c_str());

    //std::cout << s << std::endl;
    if (_score != INVALID_HANDLE_VALUE) {
        WriteFile(_score, b, s.length(), &count, NULL);
    }
}

void Sensor::SerialReadThread(void* myInstant)
{
    //std::cout << "-----------------------------------------------" << std::endl;
    G_sensor->SerialRead();
}

void Sensor::SerialRead()
{
    char b[8];
    DWORD count;

    //std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;
    while (_run) {
        if (ReadFile(_handle, b, 1, &count, NULL)) {
            ReadStateMachine(b[0]);
        }
    }
}

void Sensor::ReadStateMachine(int b)
{
    if (b == 86) { _s = 1; _v = 0; }
    else if (b == 68) { _s = 7; _d = 0; }
    else {
        switch (_s) {
        case 1: _s = 2;  _v = (b - 48); break;
        case 2: _s = 3;  _v = (_v * 10) + (b - 48); break;
        case 3: _s = 4;  _v = (_v * 10) + (b - 48); break;
        case 4: _s = 5;  _v = (_v * 10) + (b - 48); break;
        case 5: _s = 6;  _v = (_v * 10) + (b - 48); break;
        case 7: _s = 8;  _d = (b - 48); break;
        case 8: _s = 9;  _d = (_d * 10) + (b - 48); break;
        case 9: _s = 10; _d = (_d * 10) + (b - 48); break;
        default: break;
        }
    }
    if (_s == 6) {
        speed = _v;
        return;
    }
    if (_s == 10) {
        if (_d == _d_last) {
            if (_d < 14) { direction = -1; }
            else if (_d > 16) { direction = 1; }
            else { direction = 0; }
            return;
        }
        _d_last = _d;
    }
}

//---- kong ----
int  sensor_in_use(Sensor* s) { return s->in_use; }
int  sensor_direction(Sensor* s) { return s->direction; }
int  sensor_speed(Sensor* s) { return s->speed; }
void sensor_score(Sensor* s, int score) { s->SendScore(score); }
//----

/**/
