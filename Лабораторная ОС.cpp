#include <windows.h>
#include "Header.h"
#include <iostream>
#include <fstream>
#include <string>
#include <random>
#include <map>

#define KEY_SHIFTED 0x8000                 // сочетание клавиш
#define STACK_SIZE	64 * 1024              // размер буфера

int N = 0;                                 // размер поля
static char lastElem;                      // последний нарисованный элемент

struct WINDOWSIZE {                        // размер окна
    int x; int y;
} size;

struct RGB {                               // rgb цвета
    int r; int g; int b;
} background, grid, cross, zero;           // фон, сетка, крестик, нолик

static std::multimap < char, POINT > Pole; // контейнер для запоминания положения крестиков и ноликов
static HBRUSH bkBrush;                     // цвет фона
static PAINTSTRUCT ps;                     // экземпляр структуры графического вывода
static HDC hdc;                            // контекст устройства
static HWND hWnd;                          // дескриптор окна

HANDLE hMapFile;                           // отображаемый файл в памяти для работы с несколькими процессами
char* Cross_or_Zero;                       // представление файла в адресном пространстве процесса
POINT* Center;                             // ...
int* NMap;                                 // ...

const UINT UPDATE_MESSAGE = RegisterWindowMessage(L"Update");      // сообщения окна

HANDLE Process_Semaphore;                  // создание семафора для возможности работы всего двух процессов
HANDLE AnimationHNDL;                      // поток для анимации, отрисовки, проверки на завершение игры
static bool flagAnim = true;               // флаг для приостановления и возобновления анимации
static bool flagElem = false;              // флаг для определения хода игроков по очереди (true - нолик, false - крестик)
HANDLE EventStep;                          // событие для возможности хода одному игроку только крестиками, другому - только ноликами


int main(HINSTANCE hInst, int ncmdshow) {
    SetConsoleOutputCP(1251);

    hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, STACK_SIZE, L"TestMapping");
    if (!hMapFile) { std::cout << "Ошибка при CreateFileMapping\n"; return -1; }

    Cross_or_Zero = (char*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, NULL, NULL, NULL);
    Center = (POINT*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, NULL, NULL, NULL);
    NMap = (int*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, NULL, NULL, NULL);
    if (!Cross_or_Zero || !Center || !NMap) {
        std::cout << "Ошибка при MapViewOfFile\n";
        CloseHandle(hMapFile);
        return -1;
    }

    if (GetLastError() != ERROR_ALREADY_EXISTS) {
        std::string flag;
        std::cout << "Хотите оставить данные по умолчанию? (Y/N) >> "; std::cin >> flag;
        if (flag != "Y" && flag != "N") { std::cout << "Неверный ввод.\n"; return 0; }
        if (flag == "N") {
            std::string n;
            std::cout << "Размер поля N = "; std::cin >> n;
            for (int i = 0; i < n.length(); i++)
                if ((int)n[i] <= 47 || (int)n[i] >= 58) {
                    std::cout << "Неверный ввод.\n"; return 0;
                }
            N = stoi(n);
            *NMap = N;
        }
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        N = *NMap; FreeConsole();
    }

    Config_File_Work();

    if (N > 3) { size.x = N * N * 16 + 2 * N * 16; size.y = N * N * 16 + N * 8; } //размер окошка под размер сетки

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    Process_Semaphore = CreateSemaphore(NULL, 2, 2, L"Global\MySemaphore");
    if (!Process_Semaphore) Process_Semaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, L"Global\MySemaphore");

    if (Process_Semaphore && WaitForSingleObject(Process_Semaphore, 0) == WAIT_TIMEOUT) {
        CloseHandle(Process_Semaphore);
        PostQuitMessage(0);
    }

    EventStep = CreateEvent(NULL, TRUE, TRUE, L"Global\MyEvent");
    if (!EventStep) EventStep = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Global\MyEvent");

    if (EventStep && WaitForSingleObject(EventStep, 0) == WAIT_TIMEOUT) {
        SetEvent(EventStep); flagElem = true;
    }
    else
        if (EventStep) {
            ResetEvent(EventStep); flagElem = false;
        }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bkBrush = CreateSolidBrush(RGB(background.r, background.g, background.b));

    WNDCLASS SoftwareMainClass = NewWindowClass(bkBrush, LoadCursor(NULL, IDC_HAND), hInst, LoadIcon(NULL, IDI_APPLICATION), L"MainWndClass", SoftwareMainProcedure);

    if (!RegisterClassW(&SoftwareMainClass)) return -1;
    MSG SoftwareMainMessage = { 0 };

    hWnd = CreateWindow(L"MainWndClass", L"Игра Крестики-Нолики", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 600, 300, size.x, size.y, NULL, NULL, NULL, NULL);

    UpdateWindow(hWnd);
    ShowWindow(hWnd, ncmdshow);

    AnimationHNDL = CreateThread(NULL, STACK_SIZE, Animation, NULL, 0, NULL);

    while (GetMessage(&SoftwareMainMessage, NULL, NULL, NULL))
    {
        TranslateMessage(&SoftwareMainMessage);
        DispatchMessage(&SoftwareMainMessage);
    }

    UnmapViewOfFile(Cross_or_Zero); UnmapViewOfFile(Center); UnmapViewOfFile(NMap);
    CloseHandle(hMapFile); CloseHandle(Process_Semaphore); CloseHandle(AnimationHNDL); CloseHandle(EventStep);
    UnregisterClass(L"MainWndClass", hInst);
    return 0;
}

//отдельный поток для отрисовки поля
DWORD WINAPI Animation(LPVOID) {
    while (true) {
        if (flagAnim) {
            if (background.r <= 245) background.r+=10;
            else background.r = 0;
            if (background.g <= 245) background.g+=10;
            else background.g = 0;
            if (background.b <= 245) background.b+=10;
            else background.b = 0;
            COLORREF bkcolor = RGB(background.r, background.g, background.b);
            if (bkBrush) DeleteObject(bkBrush);
            bkBrush = CreateSolidBrush(bkcolor);
            SetClassLongPtr(hWnd, GCLP_HBRBACKGROUND, (LONG)bkBrush);
        }

        InvalidateRect(hWnd, NULL, TRUE);

#pragma region Paint
        Paint_Pole(true);

#pragma region GameOver
        GameOver('0'); GameOver('X');

#pragma endregion Relax
        Sleep(150);
    }
    return 0;
}

//создание окна приложения
WNDCLASS NewWindowClass(HBRUSH BGColor, HCURSOR Cursor, HINSTANCE hInst, HICON Icon, LPCWSTR Name, WNDPROC Procedure) {
    WNDCLASS NWC = { 0 };
    NWC.hIcon = Icon;
    NWC.hCursor = Cursor;
    NWC.hInstance = hInst;
    NWC.lpszClassName = Name;
    NWC.hbrBackground = BGColor;
    NWC.lpfnWndProc = Procedure;
    return NWC;
}

//обработка вызываемых процедур
LRESULT CALLBACK SoftwareMainProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    static POINT LCenter, RCenter;
    if (msg == UPDATE_MESSAGE) Invalidate_Update();
    else
        switch (msg) {

        /*case WM_PAINT: //рисование
            Paint_Pole(true);
            break;*/

     case WM_RBUTTONDOWN: //левая мышь - нолик
            if (flagElem && lastElem != '0') {
                RCenter = Center_Search(lp);
                MapViewOfFile_Work(RCenter, '0');
            }
            else 
                if (lastElem == '0') MessageBox(hWnd, L"Дождитесь своего хода", L"PauseStep", MB_OK);
                else if (!flagElem)  MessageBox(hWnd, L"Ходите в другом окне", L"OtherWindow", MB_OK);
            break;

        case WM_LBUTTONDOWN: //правая мышь - крестик
            if (!flagElem && lastElem != 'X') {
                LCenter = Center_Search(lp);
                MapViewOfFile_Work(LCenter, 'X');
            }
            else
                if (lastElem == 'X') MessageBox(hWnd, L"Дождитесь своего хода", L"PauseStep", MB_OK);
                else if (flagElem)   MessageBox(hWnd, L"Ходите в другом окне", L"OtherWindow", MB_OK);
            break;

        case WM_KEYDOWN: //клавиатура
            Key_Processing(wp);
            break;

        case WM_MOUSEWHEEL: //колесико мышки
            if (grid.r <= 255) grid.r++;
            else grid.r = 0;
            if (grid.g <= 255) grid.g++;
            else grid.g = 0;
            if (grid.b <= 255) grid.b++;
            else grid.b = 0;
            SendMessage(hWnd, UPDATE_MESSAGE, wp, lp);
            break;

        case WM_CLOSE:
        case WM_DESTROY:
            DeleteObject(hWnd); DeleteObject(hdc); DeleteObject(bkBrush); Pole.clear(); //очистка памяти
            ReleaseSemaphore(Process_Semaphore, 1, NULL);
            DeleteObject(Process_Semaphore); DeleteObject(AnimationHNDL);
            PostQuitMessage(0);
            break;

        default: return DefWindowProc(hWnd, msg, wp, lp); break;
        }
}

//рисование
void Paint_Pole(bool flag) {
    if (flag) {
        hdc = BeginPaint(hWnd, &ps);
        static HGDIOBJ original = SelectObject(hdc, GetStockObject(DC_PEN));
        SelectObject(hdc, GetStockObject(DC_PEN));
        SetDCPenColor(hdc, RGB(grid.r, grid.g, grid.b));

        if (flagElem) TextOutA(hdc, N * N * 16 + 20, 50, " Нолики", lstrlen(L" Нолики "));
        else TextOutA(hdc, N * N * 16 + 20, 50, " Крестики", lstrlen(L" Крестики "));

        for (int i = 1; i <= N; i++) {
            MoveToEx(hdc, 0, i * N * 16, NULL);
            LineTo(hdc, N * N * 16, i * N * 16);
            MoveToEx(hdc, i * N * 16, 0, NULL);
            LineTo(hdc, i * N * 16, N * N * 16);
        }

        DataOfMapFile();

        std::multimap < char, POINT >::iterator iterator;
        SetDCPenColor(hdc, RGB(zero.r, zero.g, zero.b));
        SelectObject(hdc, bkBrush);
        for (iterator = Pole.begin(); iterator != Pole.end(); iterator++) {
            POINT Center = iterator->second;
            if (iterator->first == '0')
                Ellipse(hdc, Center.x - N * 7 + 5, Center.y + N * 7 - 5, Center.x + N * 7 - 5, Center.y - N * 7 + 5);
            else
                Paint_Cross(Center, RGB(cross.r, cross.g, cross.b));
        }
        SelectObject(hdc, original);
        DeleteObject(original);
        EndPaint(hWnd, &ps);
    }
    else InvalidateRect(hWnd, NULL, TRUE); //очистка
}

//рисование крестика
void Paint_Cross(POINT Center, COLORREF Color) {
    SetDCPenColor(hdc, Color);
    MoveToEx(hdc, Center.x - N * 5, Center.y - N * 5, NULL);
    LineTo(hdc, Center.x + N * 5, Center.y + N * 5);
    MoveToEx(hdc, Center.x - N * 5, Center.y + N * 5, NULL);
    LineTo(hdc, Center.x + N * 5, Center.y - N * 5);
}

//обработка нажатия клавиш клавиатуры
void Key_Processing(WPARAM wp) {
    if (wp == VK_ESCAPE || (GetAsyncKeyState(VK_CONTROL) & KEY_SHIFTED) && (GetAsyncKeyState(0x51) & KEY_SHIFTED)) //закрытие окна
        DestroyWindow(hWnd);

    if ((GetAsyncKeyState(VK_SHIFT) & KEY_SHIFTED) && (GetAsyncKeyState(0x43) & KEY_SHIFTED)) { //открытие блокнота
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
        TCHAR str[100];
        CreateProcessW(L"C:\\Windows\\notepad.exe", str, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    }

    std::random_device random; std::mt19937 gen(random()); std::uniform_int_distribution<> dist(0, 255);
    std::wstring c;
    switch (wp) {
    case VK_RETURN: //смена цвета фона на случайный
        background.r = dist(gen), background.g = dist(gen), background.b = dist(gen);
        if (bkBrush) DeleteObject(bkBrush);
        bkBrush = CreateSolidBrush(RGB(background.r, background.g, background.b));
        SetClassLongPtr(hWnd, GCLP_HBRBACKGROUND, (LONG)bkBrush);
        InvalidateRect(hWnd, NULL, TRUE);
        break;

    case VK_SPACE://приостановление и возобновление отрисовки фона
        flagAnim = !flagAnim;
        break;

    case 0x31: //1
    case 0x61:
        SetThreadPriority(AnimationHNDL, THREAD_PRIORITY_IDLE); //уровень приотритета -15
        c = std::to_wstring(GetThreadPriority(AnimationHNDL)) + L"\nПриоритет изменен.";
        MessageBox(NULL, c.c_str(), L"Информация", MB_OK);
        break;

    case 0x32: //2
    case 0x62:
        SetThreadPriority(AnimationHNDL, THREAD_PRIORITY_TIME_CRITICAL); //уровень приоритета 15
        c = std::to_wstring(GetThreadPriority(AnimationHNDL)) + L"\nПриоритет изменен.";
        MessageBox(NULL, c.c_str(), L"Информация", MB_OK);
        break;

    case 0x33: //3
    case 0x63:
        SetThreadPriority(AnimationHNDL, THREAD_PRIORITY_NORMAL); //уровень приоритета класса 0
        c = std::to_wstring(GetThreadPriority(AnimationHNDL)) + L"\nПриоритет изменен.";
        MessageBox(NULL, c.c_str(), L"Информация", MB_OK);
        break;
    default: break;
    }
}

//проверка на завершение игры и определение победителя
void GameOver(char elem) {
    if (Check_LineOrColumnOrDiagonal(elem, 0) || Check_LineOrColumnOrDiagonal(elem, 1)
        || Check_LineOrColumnOrDiagonal(elem, 2) || Check_LineOrColumnOrDiagonal(elem, 3)) {
        Paint_Pole(false);
        elem == '0' ?  MessageBox(hWnd, L"Победила команда Ноликов", L"GameOver", MB_OK)
        /*elem == 'X'*/ : MessageBox(hWnd, L"Победила команда Крестиков", L"GameOver", MB_OK);
        SendMessage(hWnd, WM_CLOSE, 0, 0);
    }
    else
        if (Check_Pole()) {
            Paint_Pole(false);
            MessageBox(hWnd, L"Ничья", L"GameOver", MB_OK);
            SendMessage(hWnd, WM_CLOSE, 0, 0);
        }
}

//проверка на заполненность строки или столбца или диагонали одним символом
bool Check_LineOrColumnOrDiagonal(char elem, int type) {
    bool flag = false;
    std::multimap < char, POINT >::iterator iterator;
    int numberline = N * 8, countelem = 0;

    while (numberline <= (N * 8) + (N - 1) * N * 16) {
        for (iterator = Pole.begin(); iterator != Pole.end(); iterator++) {
            POINT Point = iterator->second;
            if ((type == 0 ? Point.y == numberline :                // проверка строк
                (type == 1 ? Point.x == numberline :                // проверка столбцов
                (type == 2 ? Point.x == Point.y :                   // проверка главной диагонали
                 /*type == 3*/ Point.x + Point.y == N * N * 16)))     // проверка побочной диагонали
                && iterator->first == elem) 
            {
                countelem++;
                if (countelem == N) { flag = true; return flag; }
            }
        }
        countelem = 0;
        numberline += N * 16;
    }
    return flag;
}

//проверка на заполненность поля - если полностью заполнено, то ничья
bool Check_Pole() {
    bool flag = false;
    if (Pole.size() == N * N) flag = true;
    return flag;
}

//проверка пустая ли ячейка поля
bool Not_Empty(POINT OurCenter) {
    std::multimap < char, POINT >::iterator iterator;
    bool flag = false;
    for (iterator = Pole.begin(); iterator != Pole.end(); iterator++) {
        POINT Center = iterator->second;
        if (Center.x == OurCenter.x && Center.y == OurCenter.y) { flag = true; break; }
    }
    return flag;
}

//поиск центра ячейки
POINT Center_Search(LPARAM lp) {
    POINT Center;
    Center.x = LOWORD(lp); Center.y = HIWORD(lp);
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            if ((Center.x >= i * N * 16) && (Center.x <= (i + 1) * N * 16) && (Center.y >= j * N * 16) && (Center.y <= (j + 1) * N * 16)) {
                Center.x = i * N * 16 + N * 8;
                Center.y = j * N * 16 + N * 8;
                break;
            }
    return Center;
}

//заполнение массивов для межпроцессорного взаимодействия
void MapViewOfFile_Work(POINT Point, char elem) {
    int k = N * (Point.x / (N * 16)) + Point.y / (N * 16) + 10;
    if (Cross_or_Zero[k] != '0' && Cross_or_Zero[k] != 'X') {
        Cross_or_Zero[k] = elem;
        Center[k] = Point;
    }
    SendMessage(HWND_BROADCAST, UPDATE_MESSAGE, NULL, NULL);
}

//считывание данных в multimap
void DataOfMapFile() {
    int k = 10;
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            POINT Point = Center[k];
            if ((Point.x != 0 || Point.y != 0) && Point.x <= N * N * 16 && Point.y <= N * N * 16)
                if (!Not_Empty(Point)) {
                    Pole.insert(std::make_pair(Cross_or_Zero[k], Center[k]));
                    lastElem = Cross_or_Zero[k];
                    std::cout << Cross_or_Zero[k] << " " << Point.x << " " << Point.y << std::endl;
                }
            k++;
        }
}

//работа с конфигурационным файлом
void Config_File_Work() {
    std::ifstream ifile("File.txt");
    if (!ifile.is_open()) {
        std::ofstream ofile("File.txt");
        ofile << 3 << "\n" 
            << 320 << " " << 240 << "\n" 
            << 154 << " " << 206 << " " << 235 << "\n" 
            << 200 << " " << 100 << " " << 100 << "\n"
            << 0 << " " << 0 << " " << 0 << "\n"
            << 0 << " " << 0 << " " << 0;
        ofile.close();
    }
    ifile.close();
    std::ifstream file("File.txt");
    if (N == 0) file >> N;
    else file.ignore();
    file >> size.x; file >> size.y;
    file >> background.r; file >> background.g; file >> background.b;
    file >> grid.r; file >> grid.g; file >> grid.b;
    file >> cross.r; file >> cross.g; file >> cross.b;
    file >> zero.r; file >> zero.g; file >> zero.b;
    file.close();
}

//исправление и обновление окна
void Invalidate_Update() {
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
}