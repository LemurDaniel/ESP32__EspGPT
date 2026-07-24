#include <display/historyGraph.h>
#include <display/theme.h>

HistoryGraph::HistoryGraph(const char *title, const char *unit, int size)
    : _title(title), _unit(unit), _size(size)
{
    _values = new float[_size];
}

void HistoryGraph::push(float value)
{
    _values[_next] = value;
    _next          = (_next + 1) % _size;
    if (_count < _size)
        _count++;
}

void HistoryGraph::draw(TFT_eSPI &display) const
{
    const int w = display.width();
    const int h = display.height();

    display.fillScreen(theme::background);
    display.setTextColor(theme::accent, theme::background);
    display.setTextDatum(TC_DATUM);
    display.setTextSize(2);
    display.drawString(_title, w / 2, 6);

    if (_count < 2)
    {
        display.setTextDatum(MC_DATUM);
        display.setTextSize(2);
        display.drawString("Sammle Daten...", w / 2, h / 2);
        return;
    }

    float minVal = _values[0];
    float maxVal = _values[0];
    for (int i = 0; i < _count; i++)
    {
        int idx = (_next - _count + i + _size) % _size;
        minVal  = min(minVal, _values[idx]);
        maxVal  = max(maxVal, _values[idx]);
    }
    if (maxVal - minVal < 0.5F)
    {
        minVal -= 0.5F;
        maxVal += 0.5F;
    }

    const int marginTop    = 40;
    const int marginBottom = 20;
    const int marginSide   = 34;

    const int plotX = marginSide;
    const int plotY = marginTop;
    const int plotW = w - marginSide * 2;
    const int plotH = h - marginTop - marginBottom;

    display.drawRect(plotX, plotY, plotW, plotH, theme::line);

    display.setTextColor(theme::text, theme::background);
    display.setTextDatum(ML_DATUM);
    display.setTextSize(1);
    display.drawString(String(maxVal, 1) + " " + _unit, 2, plotY);
    display.drawString(String(minVal, 1) + " " + _unit, 2, plotY + plotH);

    int prevX = -1, prevY = -1;
    for (int i = 0; i < _count; i++)
    {
        int idx = (_next - _count + i + _size) % _size;
        int x   = plotX + (_count == 1 ? 0 : (i * plotW) / (_count - 1));
        int y   = plotY + plotH - (int)(((_values[idx] - minVal) / (maxVal - minVal)) * plotH);
        if (prevX >= 0)
            display.drawLine(prevX, prevY, x, y, theme::accent);
        display.fillCircle(x, y, 2, theme::accent);
        prevX = x;
        prevY = y;
    }

    display.setTextColor(theme::line, theme::background);
    display.setTextDatum(BC_DATUM);
    display.setTextSize(1);
    display.drawString("zum Zuruck tippen", w / 2, h - 4);
}
