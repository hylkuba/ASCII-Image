/**
 * @author Jakub Hýl <hylkuba@gmail.com>
 * @date 11.01.2024
*/

#include <Windows.h>
#include <wincodec.h>

#include <iostream>
#include <vector>
#include <string>
#include <fstream>

// Compile with:
// g++ main.cpp -o IMGtoASCII -lgdi32 -luser32 -lole32 -luuid -lwindowscodecs -O2

const std::string density = "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. ";
const int densitySize = density.length();
const float indexVal = (float)256 / densitySize;

const wchar_t* imagePath = L"imgs/dog.jpg";
const std::string filePath = "ASCII/output.txt";

int main() {

    // Initialize WIC
    CoInitialize(nullptr);

    // Create WIC factory
    IWICImagingFactory* pWICFactory = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&pWICFactory)
    );

    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"Failed to initialize WIC", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Create WIC decoder
    IWICBitmapDecoder* pDecoder = nullptr;
    hr = pWICFactory->CreateDecoderFromFilename(
        imagePath,
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &pDecoder
    );

    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"Failed to create WIC decoder", L"Error", MB_OK | MB_ICONERROR);
        pWICFactory->Release();
        CoUninitialize();
        return 1;
    }

    // Load the first frame from the image
    IWICBitmapFrameDecode* pFrame = nullptr;
    hr = pDecoder->GetFrame(0, &pFrame);
    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"Failed to get image frame", L"Error", MB_OK | MB_ICONERROR);
        pDecoder->Release();
        pWICFactory->Release();
        CoUninitialize();
        return 1;
    }

    // Get image dimensions
    UINT width, height;
    hr = pFrame->GetSize(&width, &height);
    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"Failed to get image dimensions", L"Error", MB_OK | MB_ICONERROR);
        pFrame->Release();
        pDecoder->Release();
        pWICFactory->Release();
        CoUninitialize();
        return 1;
    }

    // Create a bitmap to hold the pixel data
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -static_cast<int>(height);  // Use negative height for top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;  // 24 bits per pixel (8 bits for each RGB channel)
    bmi.bmiHeader.biCompression = BI_RGB;

    // Allocate memory for the pixel data
    void* pPixels;
    HDC hdcScreen = GetDC(nullptr);
    HBITMAP hBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &pPixels, nullptr, 0);

    if (hBitmap == nullptr) {
        MessageBoxW(nullptr, L"Failed to create DIB section", L"Error", MB_OK | MB_ICONERROR);
        pFrame->Release();
        pDecoder->Release();
        pWICFactory->Release();
        CoUninitialize();
        return 1;
    }

    // Create a device context and select the bitmap
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    SelectObject(hdcMem, hBitmap);

    // Copy the image pixel data to the DIB section
    hr = pFrame->CopyPixels(nullptr, width * 3, width * height * 3, static_cast<BYTE*>(pPixels));

    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"Failed to copy pixel data", L"Error", MB_OK | MB_ICONERROR);
        DeleteObject(hBitmap);
        pFrame->Release();
        pDecoder->Release();
        pWICFactory->Release();
        CoUninitialize();
        return 1;
    }

    std::vector<std::string> ascii;

    // Process each pixel ans store to ASCII vector
    for (UINT y = 0; y < height; ++y) {
        std::string column = "";
        for (UINT x = 0; x < width; ++x) {
            // Get the color of the pixel at (x, y)
            COLORREF color = GetPixel(hdcMem, x, y);

            // Extract the RGB components
            int red = GetRValue(color);
            int green = GetGValue(color);
            int blue = GetBValue(color);

            int average = (red + green + blue) / 3;
            int index = average / indexVal;

            column += density[densitySize - index];
        }
        ascii.push_back(column);
    }

    // Save the ASCII image
    std::ofstream outputFile(filePath);

    if (outputFile.is_open()) {
        for (const std::string& line : ascii) {
            outputFile << line << std::endl;
        }

        outputFile.close();
        std::cout << "File written successfully." << std::endl;
    } else {
        std::cerr << "Error: Unable to open the file for writing." << std::endl;
    }


    // Clean up resources
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
    DeleteObject(hBitmap);
    pFrame->Release();
    pDecoder->Release();
    pWICFactory->Release();
    CoUninitialize();

    return 0;
}
