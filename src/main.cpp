//
//
//

#include <iostream>
#include "Sensor.hpp"

Sensor* G_sensor;

extern "C" {
    int tuxracer(int argc, char** argv);
}

int main(int argc, char** argv)
{
    G_sensor = new Sensor();
    if (argc > 1) {
        std::cout << argv[0] << " " << argv[1] << std::endl;
        G_sensor->SerialInit(argv[1]);
    }
    else {
        std::cout << argv[0] << std::endl;
    }
    std::cout << "hello world!\n";
    return tuxracer(argc, argv);
}

/* EOF */
