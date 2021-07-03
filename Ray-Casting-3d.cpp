#include <windows.h>
#include <stdio.h>
#include <cmath>

typedef unsigned char BYTE;
typedef struct
{
    BYTE R;
    BYTE G;
    BYTE B;
}RGB;
typedef struct
{
    BYTE B;
    BYTE G;
    BYTE R;
    BYTE A;
}RGB4;
typedef struct
{
    double x;
    double y;
    double angle;
}PLAYER;
typedef struct
{
    int x;
    int y;
} DOT_2D;

// Global vars
PLAYER player = { 25,25,0 };

DOT_2D* borders = nullptr;
int num_borders = 0;

char keyCode = -1;

double FOV = 1.0472;
int NUM_RAYS = 100;
int MAX_DEPTH = 400;

double camera_z = NUM_RAYS / 2 / tan(FOV / 2);

// Global funclions
bool is_outside(int x, int y)
{
    for (int i = 0; i < num_borders; i++)
    {
        if (x >= borders[i].x && y >= borders[i].y && x <= borders[i].x + 50 && y <= borders[i].y + 50)
        {
            return false;
        }
    }

    if (x < 0 || y < 0 || x >= 500 || y >= 500)
    {
        return false;

    }
    return true;
}
void read_map_from_file(const char* fname)
{
    FILE* fp;

    fopen_s(&fp, fname, "rt");
    if (fp)
    {
        int i, j;
        char chr;

        // Count borders and get player position
        for (i = j = 0; (chr = getc(fp)) != EOF; j++)
        {
            if (chr == '#') num_borders++;
            else if (chr == 'p') player = { (j + 0.5) * 50, (i + 0.5) * 50 };
            else if (chr == '\n') { j = -1; i++; }
        }

        // Read borders
        if (num_borders)
        {
            borders = new DOT_2D[num_borders];
            int k = 0;

            rewind(fp);

            for (i = j = 0; (chr = getc(fp)) != EOF; j++)
            {
                if (chr == '#' && k < num_borders) borders[k++] = { j * 50, i * 50 };
                if (chr == '\n') { j = -1; i++; }
            }
        }
    }
    else printf("%s read error.", fname);
}

// FRAME class
class FRAME
{
private:
    HWND hwnd = nullptr;
    HDC hdc = nullptr;
    HDC tmpDc = nullptr;
    HBITMAP hbm = nullptr;
public:
    int width;
    int height;
    RGB4* buffer = nullptr;
    RGB pen_color = {};

    FRAME(int frameWidth, int frameHeight, HWND frameHwnd)
    {
        width = frameWidth;
        height = frameHeight;
        int size = width * height;
        buffer = new RGB4[size];

        hwnd = frameHwnd;
        hdc = GetDC(hwnd);
        tmpDc = CreateCompatibleDC(hdc);
    }
    ~FRAME()
    {
        delete[] buffer;
        DeleteDC(tmpDc);
        ReleaseDC(hwnd, hdc);
    }

    void clear(RGB color = { 255,255,255 })
    {
        int i;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                i = y * width + x;
                buffer[i].R = color.R;
                buffer[i].G = color.G;
                buffer[i].B = color.B;
            }
        }
    }
    void set_pixel(int x, int y)
    {
        if (x > -1 && y > -1 && x < width && y < height)
        {
            int i = y * width + x;
            buffer[i].R = pen_color.R;
            buffer[i].G = pen_color.G;
            buffer[i].B = pen_color.B;
        }
    }
    void set_rect(int x1, int y1, int x2, int y2)
    {
        for (int y = y1; y <= y2; y++)
        {
            for (int x = x1; x <= x2; x++)
            {
                set_pixel(x, y);
            }
        }
    }
    void print()
    {
        hbm = CreateBitmap(width, height, 1, 8 * 4, buffer);

        SelectObject(tmpDc, hbm);
        BitBlt(hdc, 0, 0, width, height, tmpDc, 0, 0, SRCCOPY);

        DeleteObject(hbm);
    }
};

// Window processing function
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    if (uMessage == WM_DESTROY) PostQuitMessage(0);
    else if (uMessage == WM_KEYDOWN) keyCode = wParam;
    else if (uMessage == WM_KEYUP) keyCode = 0;
    return DefWindowProc(hwnd, uMessage, wParam, lParam);
}

int main()
{
    // Register the window class
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    WNDCLASS wc = {};
    const wchar_t CLASS_NAME[] = L"myWindow";

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    // Create the window
    const int windowWidth = 500;
    const int windowHeight = 500;
    HWND hwnd = CreateWindow(CLASS_NAME, L"Ray Casting 3d", WS_OVERLAPPEDWINDOW, 0, 0, windowWidth + 16, windowHeight + 39, nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hwnd, SW_SHOWNORMAL);
    //ShowWindow(GetConsoleWindow(), SW_SHOWNORMAL); // SW_HIDE or SW_SHOWNORMAL - Hide or Show console
    MSG msg = {};

    // Game part
    read_map_from_file("map.txt");

    FRAME frame(windowWidth, windowHeight, hwnd);

    double rx, ry, dx, dy, sin_a, cos_a, wall_h;
    int wall_x;
    BYTE col;

    // -+-+-+-+-+-+-+-+-+-+- MAIL LOOP -+-+-+-+-+-+-+-+-+-+-
    while (GetKeyState(VK_ESCAPE) >= 0)
    {
        // Processing window messages
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) break;
        }

        // Processing key event
        if (keyCode)
        {
            if (keyCode == 38)
            {
                dx = 10 * cos(player.angle);
                dy = 10 * sin(player.angle);

                if (is_outside(player.x + dx, player.y + dy))
                {
                    player.x += dx / 10;
                    player.y += dy / 10;
                }
            }
            else if (keyCode == 40)
            {
                dx = 10 * cos(player.angle);
                dy = 10 * sin(player.angle);

                if (is_outside(player.x - dx, player.y - dy))
                {
                    player.x -= dx / 10;
                    player.y -= dy / 10;
                }
            }
            else if (keyCode == 37) player.angle -= FOV / 50;
            else if (keyCode == 39) player.angle += FOV / 50;

            // Draw background
            frame.clear({});

            for (double y = 0; y < 250; y++)
            {
                col = BYTE((y / 250) * (y / 250) * 180 + 20);
                frame.pen_color = { col, col, col };

                for (int x = 0; x < 500; x++)
                {
                    frame.set_pixel(x, y + 280);
                }
            }

            // Cast walls
            wall_x = 0;
            for (double angle = -FOV / 2; angle <= FOV / 2; angle += FOV / NUM_RAYS)
            {
                sin_a = sin(player.angle + angle);
                cos_a = cos(player.angle + angle);

                for (double depth = 0; depth < MAX_DEPTH; depth++)
                {
                    rx = player.x + cos_a * depth;
                    ry = player.y + sin_a * depth;

                    if (!is_outside(rx, ry))
                    {
                        depth *= cos(angle);
                        col = BYTE((1 - depth / MAX_DEPTH) * (1 - depth / MAX_DEPTH) * 220);
                        frame.pen_color = { col, col, col };
                        wall_h = 250 * camera_z / depth;

                        frame.set_rect(wall_x, 250 - int(wall_h / 2), wall_x + 4, 250 + int(wall_h / 2));

                        break;
                    }
                }

                wall_x += 5;
            }

            // Print buffer
            frame.print();
        }
    }

    delete[] borders;

    return 0;
}