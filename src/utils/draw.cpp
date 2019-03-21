#include "draw.h"

#define IMAGE_AREA_LIMIT 16000000

#ifdef DRAW
DrawCanvas::DrawCanvas() {
    _lx = _ly = 0.0;
    _hx = _hy = 0.0;
    _imgw = 0;
    _imgh = 0;
    _fx = false;
    _fy = true;

    _image = NULL;
    _font = NULL;
}
DrawCanvas::DrawCanvas(int w, int h, bool fx, bool fy) {
    SetViewPort(w, h, 0, w, 0, h);
    _fx = fx;
    _fy = fy;
    Init();
}
DrawCanvas::DrawCanvas(int w, int h, double lx, double ly, double hx, double hy, bool fx, bool fy) {
    SetViewPort(w, h, lx, ly, hx, hy);
    _fx = fx;
    _fy = fy;
    if ((_imgw * _imgh) > IMAGE_AREA_LIMIT) {
        printf("Warning: image size too large: (%d,%d)", w, h);
    } else if ((_hx - _lx) <= 0 || (_hy - _ly) <= 0) {
        printf("Warning: viewport width and height cannot be zero");
    } else {
        _image = NULL;
        _font = NULL;
        Init();
    }
}
DrawCanvas::~DrawCanvas() { Free(); }
void DrawCanvas::SetViewPort(int w, int h, double lx, double ly, double hx, double hy) {
    _lx = min(lx, hx);
    _ly = min(ly, hy);
    _hx = max(lx, hx);
    _hy = max(ly, hy);
    if (_hx == _lx) {
        _hx = _lx + 1e-12;
    }
    if (_hy == _ly) {
        _hy = _ly + 1e-12;
    }

    if (w <= 0 && h <= 0) {
        w = _hx - _lx;
        h = _hy - _ly;
    } else if (w <= 0) {
        w = h / (_hy - _ly) * (_hx - _lx);
    } else if (h <= 0) {
        h = w / (_hx - _lx) * (_hy - _ly);
    }
    _imgw = w;
    _imgh = h;
}
void DrawCanvas::Init() {
    Free();
    _image = gdImageCreateTrueColor(_imgw, _imgh);
    _font = gdFontGetLarge();
    _lineSize = 1;
    Clear();
}
void DrawCanvas::Free() {
    if (_image) {
        gdImageDestroy(_image);
        _image = NULL;
    }
    _font = NULL;
}

#define range(x, lo, hi) (max((lo), min((hi), (x))))
void DrawCanvas::scale_to_rgb(double v, uchar *r, uchar *g, uchar *b, int key) {
    if (v < COLOR_KEY[key][0]) {
        *r = COLOR_R[key][0];
        *g = COLOR_G[key][0];
        *b = COLOR_B[key][0];
        return;
    }
    if (v > COLOR_KEY[key][COLOR_POINTS[key] - 1]) {
        *r = COLOR_R[key][COLOR_POINTS[key] - 1];
        *g = COLOR_G[key][COLOR_POINTS[key] - 1];
        *b = COLOR_B[key][COLOR_POINTS[key] - 1];
        return;
    }
    for (unsigned i = 1; i < COLOR_POINTS[key]; i++) {
        if (v <= COLOR_KEY[key][i]) {
            double rr =
                COLOR_R[key][i - 1] + ((v - COLOR_KEY[key][i - 1]) / (COLOR_KEY[key][i] - COLOR_KEY[key][i - 1]) *
                                       (COLOR_R[key][i] - COLOR_R[key][i - 1]));
            double gg =
                COLOR_G[key][i - 1] + ((v - COLOR_KEY[key][i - 1]) / (COLOR_KEY[key][i] - COLOR_KEY[key][i - 1]) *
                                       (COLOR_G[key][i] - COLOR_G[key][i - 1]));
            double bb =
                COLOR_B[key][i - 1] + ((v - COLOR_KEY[key][i - 1]) / (COLOR_KEY[key][i] - COLOR_KEY[key][i - 1]) *
                                       (COLOR_B[key][i] - COLOR_B[key][i - 1]));
            *r = (unsigned char)range((int)rr, 0, 255);
            *g = (unsigned char)range((int)gg, 0, 255);
            *b = (unsigned char)range((int)bb, 0, 255);
            return;
        }
    }
}

int DrawCanvas::getImgX(double x) {
    if (_fx) {
        return _imgw * (_hx - x) / (_hx - _lx);
    } else {
        return _imgw * (x - _lx) / (_hx - _lx);
    }
}
int DrawCanvas::getImgY(double y) {
    if (_fy) {
        return _imgh * (_hy - y) / (_hy - _ly);
    } else {
        return _imgh * (y - _ly) / (_hy - _ly);
    }
}
void DrawCanvas::Clear() { Clear(0, 1, 2); }
void DrawCanvas::Clear(int color) {
    unsigned char r = (color & 0xff0000) >> 16;
    unsigned char g = (color & 0x00ff00) >> 8;
    unsigned char b = (color & 0x0000ff);
    Clear(r, g, b);
}
void DrawCanvas::Clear(char r, char g, char b) {
    if (_image) {
        gdImageSaveAlpha(_image, 0);
        gdImageColorTransparent(_image, gdImageColorAllocate(_image, 0, 1, 2));
        SetFillColor(r, g, b);
        gdImageFilledRectangle(_image, 0, 0, _imgw, _imgh, _fillColor);
    }
}
void DrawCanvas::SetLineThickness(double size) {
    if (size == _lineSize) {
        return;
    }
    _lineSize = size;
    if (size == 0.0) {
        gdImageSetThickness(_image, 1);
    } else {
        gdImageSetThickness(_image, (int)max(1.0, size));
    }
}
void DrawCanvas::DrawLine(double x1, double y1, double x2, double y2, double size) {
    if (size >= 0.0) {
        SetLineThickness(size);
    }
    gdImageLine(_image, getImgX(x1), getImgY(y1), getImgX(x2), getImgY(y2), _lineColor);
}
void DrawCanvas::DrawRect(double x1, double y1, double x2, double y2, bool fill, double size) {
    int ix1 = getImgX(x1);
    int iy1 = getImgY(y1);
    int ix2 = getImgX(x2);
    int iy2 = getImgY(y2);
    if (fill) {
        gdImageFilledRectangle(_image, ix1, iy1, ix2, iy2, _fillColor);
    }
    if (size >= 0.0) {
        SetLineThickness(size);
        gdImageRectangle(_image, ix1, iy1, ix2, iy2, _lineColor);
    }
}
void DrawCanvas::DrawPoint(double x, double y, double size) {
    if (size <= 1.0) {
        gdImageSetPixel(_image, getImgX(x), getImgY(y), _fillColor);
    } else {
        int ix = getImgX(x);
        int iy = getImgY(y);
        int isize = ceil(0.5 * size);
        gdImageFilledRectangle(_image, ix - isize, iy - isize, ix + isize, iy + isize, _fillColor);
    }
}
void DrawCanvas::DrawString(string str, double x, double y) {
    gdImageString(_image, _font, getImgX(x), getImgY(y), (unsigned char *)str.c_str(), _lineColor);
}
void DrawCanvas::DrawNumber(int num, double x, double y) {
    stringstream ss;
    ss << num;
    DrawString(ss.str(), x, y);
}
void DrawCanvas::DrawNumber(double num, double x, double y) {
    stringstream ss;
    ss.precision(2);
    ss << num;
    DrawString(ss.str(), x, y);
}
void DrawCanvas::SetLineColor(double v, int key) {
    unsigned char r = 0, g = 0, b = 0;
    scale_to_rgb(v, &r, &g, &b, key);
    SetLineColor(r, g, b);
}
void DrawCanvas::SetFillColor(double v, int key) {
    unsigned char r = 0, g = 0, b = 0;
    scale_to_rgb(v, &r, &g, &b, key);
    SetFillColor(r, g, b);
}
void DrawCanvas::SetLineColor(unsigned char r, unsigned char g, unsigned char b) {
    _lineColor = gdImageColorAllocate(_image, r, g, b);
}
void DrawCanvas::SetFillColor(unsigned char r, unsigned char g, unsigned char b) {
    _fillColor = gdImageColorAllocate(_image, r, g, b);
}
void DrawCanvas::SetLineColor(int color) {
    unsigned char r = (color & 0xff0000) >> 16;
    unsigned char g = (color & 0x00ff00) >> 8;
    unsigned char b = (color & 0x0000ff);
    SetLineColor(r, g, b);
}
void DrawCanvas::SetFillColor(int color) {
    unsigned char r = (color & 0xff0000) >> 16;
    unsigned char g = (color & 0x00ff00) >> 8;
    unsigned char b = (color & 0x0000ff);
    SetFillColor(r, g, b);
}
void DrawCanvas::Save(string file) {
    FILE *fp;
    if (_image == NULL) {
        return;
    }
    if ((fp = fopen(file.c_str(), "wb")) == NULL) {
        return;
    }
    gdImagePng(_image, fp);
    fclose(fp);
}
#else
DrawCanvas::DrawCanvas() {}
DrawCanvas::DrawCanvas(int w, int h, bool fx, bool fy) {}
DrawCanvas::DrawCanvas(int w, int h, double lx, double ly, double hx, double hy, bool fx, bool fy) {}
DrawCanvas::~DrawCanvas() {}
void DrawCanvas::SetViewPort(int w, int h, double lx, double ly, double hx, double hy) {}
void DrawCanvas::Init() {}
void DrawCanvas::Free() {}
void DrawCanvas::scale_to_rgb(double v, uchar *r, uchar *g, uchar *b, int key) {}
int DrawCanvas::getImgX(double x) { return 0; }
int DrawCanvas::getImgY(double y) { return 0; }
void DrawCanvas::Clear() {}
void DrawCanvas::Clear(int color) {}
void DrawCanvas::Clear(char r, char g, char b) {}
void DrawCanvas::SetLineThickness(double size) {}
void DrawCanvas::DrawLine(double x1, double y1, double x2, double y2, double size) {}
void DrawCanvas::DrawRect(double x1, double y1, double x2, double y2, bool fill, double size) {}
void DrawCanvas::DrawPoint(double x, double y, double size) {}
void DrawCanvas::DrawString(string str, double x, double y) {}
void DrawCanvas::DrawNumber(int num, double x, double y) {}
void DrawCanvas::DrawNumber(double num, double x, double y) {}
void DrawCanvas::SetLineColor(double v, int key) {}
void DrawCanvas::SetFillColor(double v, int key) {}
void DrawCanvas::SetLineColor(unsigned char r, unsigned char g, unsigned char b) {}
void DrawCanvas::SetFillColor(unsigned char r, unsigned char g, unsigned char b) {}
void DrawCanvas::SetLineColor(int color) {}
void DrawCanvas::SetFillColor(int color) {}
void DrawCanvas::Save(string file) {}

#endif
