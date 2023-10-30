#include <windows.h>

#ifndef Header_h
#define Header_h

DWORD WINAPI Animation(LPVOID);																						// ��������� ����� ��� �������� ���������
WNDCLASS NewWindowClass(HBRUSH BGColor, HCURSOR Cursor, HINSTANCE hInst, HICON Icon, LPCWSTR Name, WNDPROC Procedure);	// �������� ���� ����������
LRESULT CALLBACK SoftwareMainProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);										// ����� � ���������� ��������

void Paint_Pole(bool flag);																								// ��������� ����
void Paint_Cross(POINT Center, COLORREF Color);																			// ��������� ��������

void Key_Processing(WPARAM wp);																							// ������� ������ ����������

void GameOver(char elem);																								// �������� �� ���������� ���� � ����������� ����������
bool Check_LineOrColumnOrDiagonal(char elem, int type);																	// �������� �� ������������� ������ ��� ������� ��� ��������� ����� ��������
bool Check_Pole();																										// �������� �� ������������� ���� - ���� ��������� ���������, �� �����

bool Not_Empty(POINT OurCenter);																						// �������� �� ������� ������
POINT Center_Search(LPARAM lp);																							// ����� ������ ������

void MapViewOfFile_Work(POINT Point, char elem);																		// ���������� �������� ��� ���������������� ��������������
void DataOfMapFile();																									// ���������� ������ � multimap

void Config_File_Work();																								// ������ � ���������������� ������

void Invalidate_Update();																								// ���������� � ����������� ����

#endif // !Header_h