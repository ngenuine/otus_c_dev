#ifndef KOI8R_H
#define KOI8R_H

#include <stdint.h>

extern const int rows;
extern const int cols;

// объявление, размер можно опустить, но тогда в клиентах этого заголовка нельзя будет sizeof сделать
extern const uint16_t koi8r_table[128];
extern const uint16_t cp1251_table[128];
extern const uint16_t iso8859_5_table[128];

#endif