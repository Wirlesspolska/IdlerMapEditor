#ifndef RME_COLOR_UTILS_H_
#define RME_COLOR_UTILS_H_

#include <wx/wx.h>
#include <wx/image.h>
#include <ctime>
#include <random>

class ColorUtils {
public:
    static void ShiftHue(wxImage& image, float hueShift) {
        if (!image.IsOk()) return;

        // Make sure we have alpha channel
        if (!image.HasAlpha()) {
            image.InitAlpha();
        }

        unsigned char* data = image.GetData();
        unsigned char* alpha = image.GetAlpha();
        int width = image.GetWidth();
        int height = image.GetHeight();

        for (int i = 0; i < width * height; ++i) {
            // Get alpha value first
            unsigned char a = alpha[i];
            
            // Only process pixels that are not fully transparent
            if (a > 0) {
                int r = data[i * 3];
                int g = data[i * 3 + 1];
                int b = data[i * 3 + 2];

                // Convert RGB to HSV
                float h, s, v;
                RGBtoHSV(r, g, b, h, s, v);

                // Shift hue
                h += hueShift;
                if (h >= 360.0f) h -= 360.0f;
                else if (h < 0.0f) h += 360.0f;

                // Convert back to RGB
                HSVtoRGB(h, s, v, r, g, b);

                data[i * 3] = r;
                data[i * 3 + 1] = g;
                data[i * 3 + 2] = b;
            } else {
                // For fully transparent pixels, set RGB to white to avoid black edges
                data[i * 3] = 255;
                data[i * 3 + 1] = 255;
                data[i * 3 + 2] = 255;
            }
        }
    }

    static float GetRandomHueShift() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<float> dis(0.0f, 360.0f);
        return dis(gen);
    }

private:
    static void RGBtoHSV(int r, int g, int b, float& h, float& s, float& v) {
        float rf = r / 255.0f;
        float gf = g / 255.0f;
        float bf = b / 255.0f;

        float max = std::max(std::max(rf, gf), bf);
        float min = std::min(std::min(rf, gf), bf);
        float delta = max - min;

        v = max;
        s = (max != 0.0f) ? (delta / max) : 0.0f;

        if (delta == 0.0f) {
            h = 0.0f;
        } else {
            if (max == rf) {
                h = 60.0f * fmod(((gf - bf) / delta), 6.0f);
            } else if (max == gf) {
                h = 60.0f * (((bf - rf) / delta) + 2.0f);
            } else {
                h = 60.0f * (((rf - gf) / delta) + 4.0f);
            }
        }

        if (h < 0.0f) h += 360.0f;
    }

    static void HSVtoRGB(float h, float s, float v, int& r, int& g, int& b) {
        if (s == 0.0f) {
            r = g = b = static_cast<int>(v * 255.0f);
            return;
        }

        h /= 60.0f;
        int i = static_cast<int>(floor(h));
        float f = h - i;
        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));

        float rf, gf, bf;
        switch (i % 6) {
            case 0: rf = v; gf = t; bf = p; break;
            case 1: rf = q; gf = v; bf = p; break;
            case 2: rf = p; gf = v; bf = t; break;
            case 3: rf = p; gf = q; bf = v; break;
            case 4: rf = t; gf = p; bf = v; break;
            case 5: rf = v; gf = p; bf = q; break;
        }

        r = static_cast<int>(rf * 255.0f);
        g = static_cast<int>(gf * 255.0f);
        b = static_cast<int>(bf * 255.0f);
    }
};

#endif // RME_COLOR_UTILS_H_ 