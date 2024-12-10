//#include <windows.h>
//#include <stdio.h>
//
//
//DWORD WINAPI ThreadFunction(LPVOID lpParam) {
//    printf("Поток запущен!\n");
//    while (true)
//    {
//        printf("Hello, World!");
//    }
//    return 0;
//}
//
//int main() {
//    HANDLE hThread;
//    DWORD dwThreadId;
//
//    // Создание потока
//    hThread = CreateThread(
//        NULL,                   
//        0,                      
//        ThreadFunction,         
//        NULL,                   
//        0,                      
//        &dwThreadId             
//    );
//
//    if (hThread == NULL) {
//        printf("Не удалось создать поток. Ошибка: %d\n", GetLastError());
//        return 1;
//    }
//
//
//
//    // Ожидание завершения потока
//    WaitForSingleObject(hThread, INFINITE);
//
//    // Закрытие дескриптора потока
//    CloseHandle(hThread);
//
//    printf("Поток завершен.\n");
//    return 0;
//}

#include <windows.h>
#include <stdio.h>

void CALLBACK FiberFunction(void* lpParam) {
    printf("нить запущена\n");
    for (int i = 500; i != 0; i--) {
        printf("Hello, World!\n");

        /*SwitchToFiber(lpParam);*/
    }
}

DWORD WINAPI ThreadFunction(LPVOID lpParam) {
    // Convert the thread to a fiber
    void* threadFiber = ConvertThreadToFiber(NULL);
    if (threadFiber == NULL) {
        printf("Не удалось преобразовать поток в нить. Ошибка: %d\n", GetLastError());
        return 1;
    }


    SwitchToFiber(lpParam);

    return 0;
}

int main() {
    HANDLE hThread;
    DWORD dwThreadId;

    // Convert the main thread to a fiber
    void* mainFiber = ConvertThreadToFiber(NULL);
    if (mainFiber == NULL) {
        printf("Не удалось преобразовать основной поток в нить. Ошибка: %d\n", GetLastError());
        return 1;
    }

    // Create a new fiber
    void* newFiber = CreateFiber(0, FiberFunction, NULL);
    if (newFiber == NULL) {
        printf("Не удалось создать нить. Ошибка: %d\n", GetLastError());
        return 1;
    }


    hThread = CreateThread(
        NULL,
        0,
        ThreadFunction,
        newFiber,
        0,
        &dwThreadId
    );

    if (hThread == NULL) {
        printf("Не удалось создать поток. Ошибка: %d\n", GetLastError());
        return 1;
    }

    // Wait for the thread to finish
    WaitForSingleObject(hThread, INFINITE);

    // Clean up
    CloseHandle(hThread);
    DeleteFiber(newFiber);

    printf("Поток и нить завершены.\n");
    return 0;
}