#ifndef UNICODE
#define UNICODE
#endif 

#include <windows.h>
#include <tchar.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <sstream>
#include <algorithm>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct ThreadData {
    int priority;
    double duration;
    DWORD iterations;
    double perfomance;
};

// Global array to store thread results for display
ThreadData threadResults[3];
HWND hwndGlobal;  // Global handle to the window

void displayProgress(HDC hdc, int threadIndex, DWORD iterationCount, double elapsedSeconds) {
    std::wstring text;
    std::wstringstream ss;
    ss << L"Thread " << threadIndex + 1 << L": Iterations = "
        << iterationCount << L", Elapsed = " << elapsedSeconds << L" seconds\n";
    text += ss.str();
    TextOut(hdc, 10, 20 * threadIndex + 10, text.c_str(), text.length());
}

void insertionSort(std::vector<int>& array, ThreadData& data) {
    int n = array.size();
    auto start = std::chrono::high_resolution_clock::now();

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwndGlobal, &ps);

    for (int i = 1; i < n; i++) {
        for (int j = i; j > 0 && array[j - 1] > array[j]; j--) {
            std::swap(array[j - 1], array[j]);
            data.iterations++;

            auto now = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = now - start;

            data.duration = elapsed.count();

            // Display progress every 5000 iterations for efficiency
            if (data.iterations % 5000 == 0) {
                displayProgress(hdc, data.priority, data.iterations, data.duration);
            }
        }
    }

    EndPaint(hwndGlobal, &ps);
}

DWORD WINAPI ThreadFunction(LPVOID param) {
    ThreadData* data = static_cast<ThreadData*>(param);

    const int arraySize = 10000;
    std::vector<int> array(arraySize);

    for (int i = 0; i < arraySize; ++i) {
        array[i] = arraySize - i;
    }

    insertionSort(array, *data);

    return 0;
}
//Дескриптор текущего экземпляра приложения
//pCmdLine - Командная строка, переданная приложению
//nCmdShow - флаг, указывающий, свернуто ли главное окно приложения, развернуто ли оно на весь экран или отображается в обычном режиме.
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"Sample Window Class"; //имя класса окна

    WNDCLASS wc = { };

    wc.lpfnWndProc = WindowProc; //Указатель на функцию обработки сообщений окна
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc); //Регистрация класса окна

    hwndGlobal = CreateWindowEx( //Создание окна
        0,
        CLASS_NAME,
        L"Thread Performance",
        WS_OVERLAPPEDWINDOW, //Стиль окна
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, //Размеры окна
        NULL, //Родительское окно
        NULL, //Меню
        hInstance, 
        NULL
    );

    if (hwndGlobal == NULL) {
        return 0;
    }

    ShowWindow(hwndGlobal, nCmdShow);

    // Run the threads
    const int threadCount = 3;
    HANDLE threads[threadCount]; //Массив дескрипторов потоков
    ThreadData threadData[threadCount] = {
        {0, 0.0, 0},
        {1, 0.0, 0},
        {15, 0.0, 0}
    };

    for (int i = 0; i < threadCount; ++i) {
        threads[i] = CreateThread(
            nullptr,
            0,
            ThreadFunction,
            &threadData[i],
            0,
            nullptr
        );

        if (threads[i] == nullptr) {
            MessageBox(hwndGlobal, L"Error creating thread", L"Error", MB_OK);
            return 1;
        }

        SetThreadPriority(threads[i], threadData[i].priority);
    }

    WaitForMultipleObjects(threadCount, threads, TRUE, INFINITE);

    for (int i = 0; i < threadCount; ++i) {
        CloseHandle(threads[i]);
        threadResults[i] = threadData[i];
    }

    // Refresh window to trigger WM_PAINT
    InvalidateRect(hwndGlobal, NULL, TRUE);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg); //Обрабатывает сообщения клавиатуры.
        DispatchMessage(&msg);  //Отправляет сообщение в функцию обработки сообщений окна
    }

    return 0;
}
//функция обработки сообщений окна
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY: //Указывает системе завершить цикл обработки сообщений
        PostQuitMessage(0);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps; //содержит сведения для приложения. Эти сведения можно использовать для рисования клиентской области окна, 
        //принадлежащей этому приложению
        //это функция Windows API, которая подготавливает окно к рисованию и 
        //заполняет структуру PAINTSTRUCT информацией о клиентской области окна, нуждающейся в перерисовке.
        HDC hdc = BeginPaint(hwnd, &ps);

        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

        // Draw the final thread results
        int y = 10;
        std::wstring text;
        for (int i = 0; i < 3; ++i) {
            text.clear();
            std::wstringstream ss;
            ss << L"Thread " << i + 1 << L": Priority = "
                << threadResults[i].priority << L", Duration = "
                << threadResults[i].duration << L" seconds, Iterations = "
                << threadResults[i].iterations << L", Perfomance = "
                << (threadResults[i].iterations / threadResults[i].duration) * 0.0000001;
            text += ss.str();
            TextOut(hdc, 10, y, text.c_str(), text.length());
            y += 20;
        }
        EndPaint(hwnd, &ps);
    }
                 return 0;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
    //это функция Windows API, которая обрабатывает все сообщения, которые не были обработаны 
    // в функции обработки сообщений окна (WindowProc).
}