#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

void insertionSort(std::vector<int>& array, DWORD& iterationCount) {
    int n = array.size();
    for (int i = 1; i < n; i++) {
        for (int j = i; j > 0 && array[j - 1] > array[j]; j--) {
            std::swap(array[j - 1], array[j]);
            iterationCount++;
        }
    }
}

// Function to write initial numbers to a file
void WriteInitialNumbersToFileSync(const wchar_t* filename) {
    HANDLE hFile = CreateFile(
        filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create file.\n";
        return;
    }

    const int arraySize = 10000;
    std::vector<int> numbers(arraySize);

    for (int i = 0; i < arraySize; ++i) {
        numbers[i] = arraySize - i;
    }

    DWORD bytesWritten;
    if (!WriteFile(hFile, numbers.data(), numbers.size() * sizeof(int), &bytesWritten, NULL)) {
        std::cerr << "Failed to write numbers to file.\n";
    }
    else {
        std::cout << "Initial numbers written to file.\n";
    }

    CloseHandle(hFile);
}

// Function to read numbers from a file
std::vector<int> ReadNumbersFromFile(const wchar_t* filename) {
    HANDLE hFile = CreateFile(
        filename, GENERIC_READ, 0, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open file for reading.\n";
        return {};
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    std::vector<int> numbers(fileSize / sizeof(int));

    DWORD bytesRead;
    if (!ReadFile(hFile, numbers.data(), fileSize, &bytesRead, NULL)) {
        std::cerr << "Failed to read numbers from file.\n";
    }
    else {
        std::cout << "Numbers read from file.\n";
    }

    CloseHandle(hFile);
    return numbers;
}

// Function to write sorted numbers back to a file
void WriteSortedNumbersToFile(const wchar_t* filename, const std::vector<int>& numbers) {
    HANDLE hFile = CreateFile(
        filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open file for writing.\n";
        return;
    }

    DWORD bytesWritten;
    if (!WriteFile(hFile, numbers.data(), numbers.size() * sizeof(int), &bytesWritten, NULL)) {
        std::cerr << "Failed to write sorted numbers to file.\n";
    }
    else {
        std::cout << "Sorted numbers written to file.\n";
    }

    CloseHandle(hFile);
}

// Write in file
void WriteInitialNumbersToFileAsync(const wchar_t* filename) {
    HANDLE hFile = CreateFile(
        filename,                    // file name
        GENERIC_WRITE,               // write mode
        0,                           // common access
        NULL,                        // security attributes
        CREATE_ALWAYS,               // create file or overwrite
        FILE_ATTRIBUTE_NORMAL,       // file attributes
        NULL                         // file pattern
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create file.\n";
        return;
    }

    /*std::vector<int> numbers = { 12, 45, 7, 89, 23, 56, 1, 78 };*/

    const int arraySize = 10000;
    std::vector<int> numbers(arraySize);

    for (int i = 0; i < arraySize; ++i) {
        numbers[i] = arraySize - i;
    }
    
    // Binary format
    DWORD bytesWritten;
    if (!WriteFile(hFile, numbers.data(), numbers.size() * sizeof(int), &bytesWritten, NULL)) {
        std::cerr << "Failed to write numbers to file.\n";
    }
    else {
        std::cout << "Initial numbers written to file.\n";
    }

    // CLose File
    CloseHandle(hFile);
}

DWORD WINAPI AsyncRead(LPVOID lpParam) {
    HANDLE hFile = *(HANDLE*)lpParam;  //Get file descriptor
    OVERLAPPED ol = { 0 };
    ol.Offset = 0; //where to start reading
    ol.OffsetHigh = 0;

    const int BUF_SIZE = 4096;
    char buffer[BUF_SIZE];

    DWORD bytesRead = 0;
    BOOL result = ReadFile(hFile, buffer, BUF_SIZE, &bytesRead, &ol);

    if (!result && GetLastError() != ERROR_IO_PENDING) {
        std::cerr << "Failed to read file asynchronously.\n";
        return 1;
    }

    // Wait for the operation to complete
    if (!GetOverlappedResult(hFile, &ol, &bytesRead, TRUE)) {
        std::cerr << "Failed to get overlapped result.\n";
        return 1;
    }

    std::cout << "Read " << bytesRead << " bytes.\n";

    // Convert buffer to integer array for sorting
    std::vector<int> data;
    for (size_t i = 0; i < bytesRead / sizeof(int); ++i) {
        data.push_back(((int*)buffer)[i]);
    }

    // before sorting
    /*std::cout << "Data read from file (before sorting):\n";
    for (const auto& num : data) {
        std::cout << num << " ";
    }
    std::cout << std::endl;*/

    // sort
    DWORD iterationCount = 0;
    insertionSort(data, iterationCount);

    std::cout << "Sorting completed. Iterations: " << iterationCount << "\n";

    WriteSortedNumbersToFile(L"datafile.dat", data);

    // Sort Numbers
    //std::cout << "Data after sorting:\n";
    //for (const auto& num : data) {
    //    std::cout << num << " ";
    //}
    //std::cout << std::endl;

    return 0;
}

DWORD WINAPI AsyncWrite(LPVOID lpParam) {
    HANDLE hFile = *(HANDLE*)lpParam;
    OVERLAPPED ol = { 0 };
    ol.Offset = 0;
    ol.OffsetHigh = 0;

    const int BUF_SIZE = 4096;
    char buffer[BUF_SIZE];

    DWORD bytesWritten = 0;
    BOOL result = WriteFile(hFile, buffer, BUF_SIZE, &bytesWritten, &ol);

    if (!result && GetLastError() != ERROR_IO_PENDING) {
        std::cerr << "Failed to write file asynchronously.\n";
        return 1;
    }

    // Wait for the operation to complete
    if (!GetOverlappedResult(hFile, &ol, &bytesWritten, TRUE)) {
        std::cerr << "Failed to get overlapped result.\n";
        return 1;
    }

    std::cout << "Wrote " << bytesWritten << " bytes.\n";
    return 0;
}

int main() {
    const wchar_t* filename = L"datafile.dat";

    // Write numbers in file
    WriteInitialNumbersToFileSync(filename);

    auto startSync = std::chrono::high_resolution_clock::now();

    std::vector<int> numbers = ReadNumbersFromFile(filename);
    DWORD iterationCount = 0;
    insertionSort(numbers, iterationCount);
    WriteSortedNumbersToFile(filename, numbers);
    auto endSync = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsedSync = endSync - startSync;
    std::cout << "Sync method: " << elapsedSync.count() << " seconds.\n";
    std::cout << "Perfomance: " << static_cast<double>((iterationCount) / elapsedSync.count()) * 0.0000000001 << "\n";


    WriteInitialNumbersToFileAsync(filename);
    // Open File for async write and read
    HANDLE hFile = CreateFile(
        filename,                   
        GENERIC_READ | GENERIC_WRITE,
        0,                           
        NULL,                        
        OPEN_EXISTING,               
        FILE_FLAG_OVERLAPPED,        // async input/output
        NULL                         
    );


    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open file.\n";
        return 1;
    }

    auto startAsync = std::chrono::high_resolution_clock::now();

    // Start async read and write
    HANDLE hReadThread = CreateThread(NULL, 0, AsyncRead, &hFile, 0, NULL);
    HANDLE hWriteThread = CreateThread(NULL, 0, AsyncWrite, &hFile, 0, NULL);

    // Waiting
    WaitForSingleObject(hReadThread, INFINITE);
    WaitForSingleObject(hWriteThread, INFINITE);

    auto endAsync = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsedAsync = endAsync - startAsync;
    std::cout << "Async method: " << elapsedAsync.count() << " seconds.\n";
    std::cout << "Perfomance: " << static_cast<double>((iterationCount) / elapsedAsync.count()) * 0.0000000001 << "\n";


    CloseHandle(hFile);
    return 0;
}
