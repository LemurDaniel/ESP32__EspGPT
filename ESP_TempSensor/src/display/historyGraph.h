#pragma once

#include <TFT_eSPI.h>

class HistoryGraph
{
public:
    HistoryGraph(const char *title, const char *unit, int size = 120);

    void push(float value);
    void draw(TFT_eSPI &display) const;

private:
    const char *_title;
    const char *_unit;
    int _size;
    float *_values;
    int _count = 0;
    int _next  = 0;
};
