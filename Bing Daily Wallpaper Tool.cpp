#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <exception>

#include <windows.h>
#include <libloaderapi.h>
#include <tchar.h>

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
bool downloadToFile(char url[], char filePath[])
{
    DeleteUrlCacheEntry_custom DeleteCache = (DeleteUrlCacheEntry_custom)GetProcAddress(WinInet, "DeleteUrlCacheEntryA");
    DeleteCache(url);

    UrlDownloadToFile_custom Download = (UrlDownloadToFile_custom)GetProcAddress(UrlMon, "URLDownloadToFileA");
    Download(NULL, url, filePath, 0, NULL);

    return true;
}

inline void stringToCharArray(std::string s, char x[])
{
    for (int i = 0; i < s.size(); ++i)
        x[i] = s[i];
}

int main()
{
    std::cout << "Welcome to the Bing Daily Wallpaper Tool."
              << " Today's Bing Wallpaper is on it's way to your desktop background.\n\n"
              << "\tPlease wait...\n"
              << std::endl;

    std::string _xmlFilePath = fs::current_path().string() + "\\temp.xml";
    std::string _imageFilePath = fs::current_path().string() + "\\todaysbingimage.jpg";

    char xmlDataUrl[] = "https://www.bing.com/HPImageArchive.aspx?format=xml&idx=0&n=1&mkt=en-US";
    char xmlFilePath[_xmlFilePath.size()];
    char imageFilePath[_imageFilePath.size()];

    stringToCharArray(_xmlFilePath, xmlFilePath);
    stringToCharArray(_imageFilePath, imageFilePath);

    if (!downloadToFile(xmlDataUrl, xmlFilePath))
    {
        std::cout << "Error at XML data download." << std::endl;
        return 0;
    }

    // here we extract the URL from the XML data.
    std::string data = "", tmp, imageUrl = "https://bing.com";
    std::ifstream fin(xmlFilePath);
    while (fin >> tmp)
        data += tmp;
    std::size_t urlMarkers[2] = {data.find("<url>"), data.find("</url>")};
    for (int i = urlMarkers[0] + 5; i < urlMarkers[1]; ++i)
        imageUrl += data[i];

    if (!downloadToFile(&(*imageUrl.begin()), imageFilePath))
    {
        std::cout << "Error at Today's Image download." << std::endl;
        return 0;
    }

    // the final step
    if (setDesktopBackground(imageFilePath))
        std::cout << "Successfully added as wallpaper.\n";
    else
        std::cout << "There was some error in setting the wallpaper.\n";

    std::cout << "Press any key to continue." << std::endl;
    getchar();

    return 0;
}

//! sources of help:
// https://stackoverflow.com/questions/10639914/is-there-a-way-to-get-bings-photo-of-the-day
// http://www.cplusplus.com/forum/windows/108182/
// https://stackoverflow.com/questions/27905108/download-file-from-url-dev-c-implementation-using-loadlibrary