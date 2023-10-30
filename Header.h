#include <windows.h>

#ifndef Header_h
#define Header_h

DWORD WINAPI Animation(LPVOID);																						// отдельный поток дл€ анимации отрисовки
WNDCLASS NewWindowClass(HBRUSH BGColor, HCURSOR Cursor, HINSTANCE hInst, HICON Icon, LPCWSTR Name, WNDPROC Procedure);	// создание окна приложени€
LRESULT CALLBACK SoftwareMainProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);										// вызов и выполнение процедур

void Paint_Pole(bool flag);																								// рисование пол€
void Paint_Cross(POINT Center, COLORREF Color);																			// рисование крестика

void Key_Processing(WPARAM wp);																							// нажатие клавиш клавиатуры

void GameOver(char elem);																								// проверка на завершение игры и определение победител€
bool Check_LineOrColumnOrDiagonal(char elem, int type);																	// проверка на заполненность строки или столбца или диагонали одним символом
bool Check_Pole();																										// проверка на заполненность пол€ - если полностью заполнено, то ничь€

bool Not_Empty(POINT OurCenter);																						// проверка на пустоту €чейки
POINT Center_Search(LPARAM lp);																							// поиск центра €чейки

void MapViewOfFile_Work(POINT Point, char elem);																		// заполнение массивов дл€ межпроцессорного взаимодействи€
void DataOfMapFile();																									// считывание данных в multimap

void Config_File_Work();																								// работа с конфигурационным файлом

void Invalidate_Update();																								// обновление и исправление окна

#endif // !Header_h