//
// Created by Cobeasta on 3/13/2024.
//

#ifndef HELLO_WORLD_DHT_H
#define HELLO_WORLD_DHT_H

#define DHT_OK 0
#define DHT_CHECKSUM_ERROR -1
#define DHT_TIMEOUT_ERROR -2

// == function prototypes =======================================

void read_sensor(float * temperature, float * humidity);
