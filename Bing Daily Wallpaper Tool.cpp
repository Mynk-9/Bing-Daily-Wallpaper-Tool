// Copyright (C) 2020 Mayank Mathur (Mynk-9)

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <chrono>

#include <windows.h>
#include <libloaderapi.h>

namespace fs = std::filesystem;

// dynamically loading libraries
HINSTANCE UrlMon = LoadLibraryA("urlmon.dll");
HINSTANCE WinInet = LoadLibraryA("wininet.dll");
// type defining two functions which we will load from the two prev.
// libraries.
typedef HRESULT *(*UrlDownloadToFile_custom)(void *, const char *, const char *, DWORD, void *);
typedef WINBOOL *(*DeleteUrlCacheEntry_custom)(LPCSTR);

bool setDesktopBackground(std::string path)
{
    int result = SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, (PVOID *)path.c_str(), SPIF_UPDATEINIFILE);

    if (result)
        return true;
    else
        return false;
}
void downloadToFile(char url[], char filePath[])
{
    DeleteUrlCacheEntry_custom DeleteCache = (DeleteUrlCacheEntry_custom)GetProcAddress(WinInet, "DeleteUrlCacheEntryA");
    DeleteCache(url);

    UrlDownloadToFile_custom Download = (UrlDownloadToFile_custom)GetProcAddress(UrlMon, "URLDownloadToFileA");
    Download(NULL, url, filePath, 0, NULL);
}

// convert std::string to terminated char array
inline void stringToCharArray(std::string s, char x[])
{
    int i;
    for (i = 0; i < s.size(); ++i)
        x[i] = s[i];
    x[i] = '\0';
}

std::string formatTodaysDate(char *date)
{
    std::string _date = "";
    for (int i = 0;; ++i)
    {
        if (date[i] == ':')
            date[i] = '-';
        else if (date[i] == ' ')
            date[i] = '_';
        else if (date[i] == '\n')
            break;

        _date += date[i];
    }
    return _date;
}
void printLicenseTerms()
{
    std::cout << "\t**************************************************\n"
              << "\t* Copyright (c) 2020 Mayank Mathur (MIT License) *\n"
              << "\t* Code can be found at mynk-9@github             *\n"
              << "\t**************************************************\n";
    std::cout << "\n\n";
    std::cout << std::flush;
}
int terminateProgram()
{
    std::cout << "\nPress any key to continue." << std::endl;
    getchar();

    return 0;
}

int main()
{
    printLicenseTerms();

    std::cout << "Welcome to the Bing Daily Wallpaper Tool."
              << " Today's Bing Wallpaper is on it's way to your desktop background.\n\n"
              << "\tPlease wait...\n"
              << std::endl;

    // Get todays date by getting time_point from system_clock::now() and converting it to
    // time_t, now converting this time_t to char* using ctime function.
    // ctime returns a \n terminated char array instead of \0. We convert this to std::string.
    std::time_t timenow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string todaysbingimage = formatTodaysDate(std::ctime(&timenow)) + ".jpg";

    std::string _xmlFilePath = fs::current_path().string() + "\\temp.xml";
    std::string _imageFilePath = fs::current_path().string() + "\\images\\" + todaysbingimage;

    char xmlDataUrl[] = "https://www.bing.com/HPImageArchive.aspx?format=xml&idx=0&n=1&mkt=en-US";
    char xmlFilePath[_xmlFilePath.size() + 1];
    char imageFilePath[_imageFilePath.size() + 1];

    stringToCharArray(_xmlFilePath, xmlFilePath);
    stringToCharArray(_imageFilePath, imageFilePath);

    try
    {
        downloadToFile(xmlDataUrl, xmlFilePath); // image path data downloaded
    }
    catch (int e)
    {
        std::cout << "Error at data download." << std::endl;

        return terminateProgram();
    }

    // here we extract the URL from the XML data.
    std::string data = "", tmp, imageUrl = "https://bing.com";
    try
    {
        std::ifstream fin;
        fin.open(xmlFilePath);
        if (!fin) // check if the file exists
            throw(-1);

        while (fin >> tmp) // load the data in file
            data += tmp;
        fin.close(); // file closed

        std::size_t urlMarkers[2] = {data.find("<url>"), data.find("</url>")};
        for (int i = urlMarkers[0] + 5; i < urlMarkers[1]; ++i)
            imageUrl += data[i];
    }
    catch (int e)
    {
        std::cout << "There was some error. Please check your internet connection.";

        return terminateProgram();
    }
    catch (...) // default exception catching
    {
        std::cout << "Defeult exception occured.";

        return terminateProgram();
    }

    try
    {
        downloadToFile(&(*imageUrl.begin()), imageFilePath); // image downloaded
    }
    catch (int e)
    {
        std::cout << "Error at Today's Image download." << std::endl;

        return terminateProgram();
    }

    // Deletions added to solve a bug where the download function
    // would try to load the image from cache and not update current day's image.
    // And it solved another purpose to clear up the clutter.
    DeleteFileA(_xmlFilePath.c_str());

    // the final step
    if (setDesktopBackground(imageFilePath))
    {
        std::cout << "Successfully added as wallpaper.\n\n";
    }
    else
    {
        std::cout << "There was some error in setting the wallpaper.\n";

        return terminateProgram();
    }

    std::cout << "Do you wish to save today's image? (press Y/y for yes): ";
    char saveImageAnswer;
    std::cin >> saveImageAnswer;

    if (saveImageAnswer == 'Y' || saveImageAnswer == 'y') // check if image has to be saved
        std::cout << "The image " << todaysbingimage << " is saved.\n";
    else
        // Deletions added to solve a bug where the download function
        // would try to load the image from cache and not update current day's image.
        // And it solved another purpose to clear up the clutter.
        DeleteFileA(_imageFilePath.c_str());

    return terminateProgram();
}

//! sources of help:
// https://stackoverflow.com/questions/10639914/is-there-a-way-to-get-bings-photo-of-the-day
// http://www.cplusplus.com/forum/windows/108182/
// https://stackoverflow.com/questions/27905108/download-file-from-url-dev-c-implementation-using-loadlibrary