#pragma once

#include <gd.h>
#include <gdfontt.h>
#include <gdfonts.h>
#include <gdfontmb.h>
#include <gdfontl.h>
#include <gdfontg.h>

#include <cmath>
#include <string>
#include <sstream>
using namespace std;

#define uchar unsigned char

class DrawCanvas {
private:
    gdImagePtr _image;
    gdFontPtr _font;
    int _lineColor;
    int _fillColor;
    double _lineSize;

    double COLOR_KEY[4][10] = {{0.000, 0.125, 0.250, 0.375, 0.500, 0.625, 0.750, 0.875, 1.000},
                               {0.000, 0.501, 0.501, 1.000},
                               {0.000, 0.501, 0.501, 1.000},
                               {0.000, 1.000}};
    double COLOR_R[4][10] = {{0, 0, 0, 0, 128, 255, 255, 255, 128}, {0, 0, 255, 255}, {0, 128, 128, 255}, {0, 255}};
    double COLOR_G[4][10] = {{0, 0, 128, 255, 255, 255, 128, 0, 0}, {0, 255, 255, 0}, {0, 128, 0, 0}, {0, 255}};
    double COLOR_B[4][10] = {{128, 255, 255, 255, 128, 0, 0, 0, 0}, {255, 0, 0, 0}, {0, 128, 0, 0}, {0, 255}};
    unsigned COLOR_POINTS[4] = {9, 4, 4, 2};

protected:
    double _lx, _ly;
    double _hx, _hy;
    int _imgw;
    int _imgh;
    bool _fx;
    bool _fy;

private:
    int getImgX(double x);
    int getImgY(double y);

public:
    enum Font { FontTiny, FontSmall, FontMediumBold, FontLarge, FontGiant };

    DrawCanvas();
    DrawCanvas(int w, int h, bool fx = false, bool fy = false);
    DrawCanvas(int w, int h, double lx, double ly, double hx, double hy, bool fx = false, bool fy = true);
    ~DrawCanvas();

    void SetViewPort(int w, int h, double lx, double ly, double hx, double hy);
    void Init();
    void Free();

    void scale_to_rgb(double v, uchar *r, uchar *g, uchar *b, int key);

    void Clear();
    void Clear(int color);
    void Clear(char r, char g, char b);
    void DrawLine(double x1, double y1, double x2, double y2, double size = -1.0);
    void DrawRect(double x1, double y1, double x2, double y2, bool fill = true, double size = -1.0);
    void DrawPoint(double x, double y, double size = 0.0);
    void DrawString(string str, double x, double y);
    void DrawNumber(int num, double x, double y);
    void DrawNumber(double num, double x, double y);
    void SetLineColor(double v, int key);
    void SetFillColor(double v, int key);
    void SetLineColor(unsigned char r, unsigned char g, unsigned char b);
    void SetFillColor(unsigned char r, unsigned char g, unsigned char b);
    void SetLineColor(int color);
    void SetFillColor(int color);
    void SetLineThickness(double size);
    void SetFont(Font size);
    void Save(string file);
};