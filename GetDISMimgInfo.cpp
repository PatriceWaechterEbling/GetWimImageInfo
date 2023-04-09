// GetDISMimgInfo.cpp : Ce fichier contient la fonction 'main'. L'exécution du programme commence et se termine à cet endroit.
//
#define WIN32_LEAN_AND_MEAN	
#define titre "Assistant d'information images WIM/VHD"
#define WINVER 0x600

#include <iostream>

#include "windows.h"
#include <windowsx.h>
#include <wingdi.h>
#include <commctrl.h>
#include <Winuser.h>
#include <Commdlg.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <stdio.h>
#include <conio.h>
#include <iostream>
#include <stdlib.h>
#include <strsafe.h>
#include <ShellAPI.h>
#include <io.h>	
#include <Shobjidl.h>
#include <Objbase.h>
#include <tCHAR.h>
#include "DismApi.h"
#include "resource.h"

#pragma comment (lib,"comctl32.lib") 
#pragma comment (lib,"kernel32.lib") 
#pragma comment (lib,"user32.lib") 
#pragma comment (lib,"gdi32.lib") 
#pragma comment (lib,"winspool.lib") 
#pragma comment (lib,"comdlg32.lib") 
#pragma comment (lib,"advapi32.lib") 
#pragma comment (lib,"shell32.lib") 
#pragma comment (lib,"ole32.lib") 
#pragma comment (lib,"oleaut32.lib") 
#pragma comment (lib,"uuid.lib") 
#pragma comment (lib,"odbc32.lib") 
#pragma comment (lib,"odbccp32.lib") 
#pragma comment (lib,"shlwapi.lib") 
#pragma comment (lib,"dismapi")
#pragma warning(disable:4996)

BROWSEINFO bi;
ITEMIDLIST* il;
OPENFILENAMEA ofn;
INITCOMMONCONTROLSEX iccex;
SYSTEMTIME st;
HWND hWnd;
HMODULE hMainMod;
CHAR szPath[MAX_PATH];
CHAR appPath[MAX_PATH];
CHAR buffer[MAX_PATH];
CHAR Result[MAX_PATH];
CHAR szFile[MAX_PATH];
CHAR jours[7][10] = { "dimanche", "lundi","mardi","mercredi","jeudi","vendredi","samedi" };
CHAR mois[12][10] = { "janvier", "fevrier","mars", "avri", "mai", "juin","juillet","aout","septembre", "octobre", "novembre", "decembre" };
CHAR statut[2][12] = { "Desactivee","Activee" };
CHAR date[0x08];
CHAR nom[0x30];
CHAR Description[0x30];
DWORD version = MAKEWORD(21, 1);
CHAR edition[0x16];
INT type = 0;
TCHAR volumeName[MAX_PATH + 1];
TCHAR fileSysName[MAX_PATH + 1];
TCHAR driveType[MAX_PATH];
HICON ico;
/// <summary>
/// specialement pour DISM/VHD
HRESULT hr,hrLocal;
DismSession session;
BOOL bMounted;
DWORD dwUnmountFlags;
DismImageInfo* pImageInfo;
DismPackage* pPackage;
DismPackageInfo* pPackageInfo;
DismFeature* pFeatures;
UINT uiCount;
const DWORD RUN_ACTION_SHELLEX_FAILED = 0xFFFFFFFFFFFFFFFF;
const DWORD RUN_ACTION_SUCCESSFUL = 0x400;
const DWORD RUN_ACTION_CANCELLED = 0xC000013A;
/// </summary>
unsigned __int64 i64NumberOfBytesUsed;

void ChargerImage();
void Splash();
void initDISM();
int enumAgument(int argc, char** argv);
VOID ExchangeColors(CHAR* Message, INT CouleurMessage, CHAR* Notification, INT CouleurNotification);
CHAR* GetDate();
BOOL WINAPI SetConsoleIcon(HICON hIcon);
DWORD GetVolumeInfo(LPCTSTR pDriveLetter, LPTSTR pDriveInfoBuffer, DWORD nDriveInfoSize);
VOID GetDiskSpaces(LPCTSTR pDriveLetter, LPTSTR pSpaceInfoBuffer, DWORD nSpaceInfoBufferSize);
CHAR* HarmoniseNom(CHAR* texte);
CHAR* CreerDescription(CHAR* texte);
INT MsgBox(CHAR* lpszText, CHAR* lpszCaption, DWORD dwStyle, INT lpszIcon);

int _cdecl main(int argc, char* argv[])
{
    Splash();
    initDISM();
    enumAgument(argc, argv);
    // Initialize the API
    hr = DismInitialize(DismLogErrorsWarningsInfo, L"C:\\MyLogFile.txt", NULL);
    if (FAILED(hr))
    {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0F | FOREGROUND_INTENSITY);
        printf("Initialisation a retourne l'erreur: 0x%x\n", hr);
        goto Cleanup;
    }

  //  ExchangeColors((char*)ofn.lpstrTitle,15,ofn.lpstrFile,10);

    // Mount a VHD image
    hr = DismMountImage(L"C:\\Install.VHD",
        L"C:\\MountPath",
        1,
        NULL,
        DismImageIndex,
        DISM_MOUNT_READWRITE,
        NULL,
        NULL,
        NULL);
    if (FAILED(hr))
    {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x4F | FOREGROUND_INTENSITY);
        printf("\nLe demontage a retourne 0x%x comme message d'erreur\n", hr);
        goto Cleanup;
    }

    bMounted = TRUE;
    printf("Operation reussie.\n\n");

    // Get some information about the image that was mounted
    hr = DismGetImageInfo((PCWSTR)szFile,
        &pImageInfo,
        &uiCount);
    if (FAILED(hr))
    {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x4F | FOREGROUND_INTENSITY);
        printf("\nLa commande lire informations a retourne le code 0x%x\n", hr);
        goto Cleanup;
    }

    // Print out some information from the image
    printf("\n\nHere is some information about this image:\n\n");
    for (UINT i = 0; i < uiCount; ++i)
    {
        printf("Image index: %u\n", i);
        printf("OS Version: %u.%u.%u.%u\n", pImageInfo[i].MajorVersion, pImageInfo[i].MinorVersion, pImageInfo[i].Build, pImageInfo[i].SpBuild);
        printf("Architecture: %u\n\n", pImageInfo[i].Architecture);
    }

    // Open a session against the mounted image
    hr = DismOpenSession(L"C:\\MountPath",
        NULL,
        NULL,
        &session);
    if (FAILED(hr))
    {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x4F | FOREGROUND_INTENSITY);
        printf("La commande ouverture de session a retourne le code 0x%x\n", hr);
        goto Cleanup;
    }

    // Get a list of all of the packages installed in the image
    hr = DismGetPackages(session,
        &pPackage,
        &uiCount);
    if (FAILED(hr))
    {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x4F | FOREGROUND_INTENSITY);
        printf("La commande lecture paquet a retourne le code 0x%x\n", hr);
        goto Cleanup;
    }

    // If there is at least one package, then get some extended information
    // about that package
    if (uiCount > 0)
    {
        printf("%u paquetages ont ete trouves \n", uiCount);
        printf("Getting more detailed information about the first package\n\n");
        hr = DismGetPackageInfo(session,
            pPackage[0].PackageName,
            DismPackageName,
            &pPackageInfo);
        if (FAILED(hr))
        {
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x4F | FOREGROUND_INTENSITY);
            printf("La commande requete de paquet a retourne le code 0x%x\n", hr);
            goto Cleanup;
        }

        printf("Horrodatage du paquet:\n");
        printf("Annee:%u\n", pPackageInfo->InstallTime.wYear);
        printf("Mois:%u\n", pPackageInfo->InstallTime.wMonth);
        printf("Jour:%u\n", pPackageInfo->InstallTime.wDay);
    }

    // Add a package (this could be a hotfix)
    hr = DismAddPackage(session,L"c:\\AddressBook.cab",
        FALSE,
        FALSE,
        NULL,
        NULL,
        NULL);
    if (FAILED(hr))
    {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x4F | FOREGROUND_INTENSITY);
        printf("La commande ajout de paquet a retourne le code 0x%x\n", hr);
        goto Cleanup;
    }

    // If the package was successfully added, then commit the image later when
    // it is unmounted
    dwUnmountFlags = DISM_COMMIT_IMAGE;

    // Get a list of all of the features in the image
    hr = DismGetFeatures(session,
        NULL,
        DismPackageNone,
        &pFeatures,
        &uiCount);
    if (FAILED(hr))
    {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x4F | FOREGROUND_INTENSITY);
        printf("La requete d'obtention a retourne le code: 0x%x\n", hr);
        goto Cleanup;
    }

    //Print out all of the feature names in the image
    printf("\n\nFonctionnalitees contenue dans l'image:\n\n");
    for (UINT i = 0; i < uiCount; ++i)
    {
        printf("Fonctionnalite: %s\n", pFeatures[i].FeatureName);
    }

Cleanup:

    // Delete the memory associated with the objects that were returned
    hrLocal = DismDelete(pImageInfo);
    if (FAILED(hrLocal))
    {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x4F | FOREGROUND_INTENSITY);
        printf("Echec lors de la supression : %x\n", hrLocal);
    }

    hrLocal = DismDelete(pPackage);
    if (FAILED(hrLocal))
    {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x4F | FOREGROUND_INTENSITY);
        printf("DismDelete Failed: %x\n", hrLocal);
    }

    hrLocal = DismDelete(pPackageInfo);
    if (FAILED(hrLocal))
    {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x4F | FOREGROUND_INTENSITY);
        printf("DismDelete Failed: %x\n", hrLocal);
    }

    hrLocal = DismDelete(pFeatures);
    if (FAILED(hrLocal))
    {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x4F | FOREGROUND_INTENSITY);
        printf("DismDelete Failed: %x\n", hrLocal);
    }

    // Close the DismSession to free up resources tied to this image servicing session
    hrLocal = DismCloseSession(session);
    if (FAILED(hrLocal))
    {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x4F | FOREGROUND_INTENSITY);
        printf("DismCloseSession Failed: %x\n", hrLocal);
    }

    // Unmount the image if it is mounted.  If the package was added successfully,
    //then commit the changes.  Otherwise, discard the
    // changes
    if (bMounted)
    {
        hrLocal = DismUnmountImage(L"C:\\MountPath",
            dwUnmountFlags,
            NULL,
            NULL,
            NULL);
        if (FAILED(hrLocal))
        {
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x4F | FOREGROUND_INTENSITY);
            printf("DismUnmountImage Failed: %x\n", hrLocal);
        }
    }

    // Shutdown the DISM API to free up remaining resources
    hrLocal = DismShutdown();
    if (FAILED(hrLocal))
    {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x4F | FOREGROUND_INTENSITY);
        printf("DismShutdown Failed: %x\n", hr);
    }
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED|FOREGROUND_BLUE| FOREGROUND_GREEN | FOREGROUND_INTENSITY);

    printf("Return code is: %x\n", hr);
    return hr;
}
VOID ExchangeColors(CHAR* Message, INT CouleurMessage, CHAR* Notification, INT CouleurNotification) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, CouleurNotification);
    printf(Notification);
    SetConsoleTextAttribute(hConsole, CouleurMessage);
    printf(Message);
}

void ChargerImage()
{
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = 0x104;
    ofn.lpstrInitialDir = szPath;
    ofn.lpstrFilter = "Fichiers Images\0*.wim;*.vhd\0\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = "Charger une image";
    ofn.lpstrFileTitle = nom;
    ofn.nMaxFileTitle = 0x104;
    DWORD flags = OFN_READONLY | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_EXTENSIONDIFFERENT | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_SHAREAWARE |
        OFN_NONETWORKBUTTON | OFN_NOLONGNAMES | OFN_EXPLORER | OFN_NODEREFERENCELINKS | OFN_LONGNAMES | OFN_ENABLEINCLUDENOTIFY | OFN_ENABLESIZING |
        OFN_ENABLEINCLUDENOTIFY | OFN_ENABLESIZING | OFN_DONTADDTORECENT | OFN_FORCESHOWHIDDEN;
    ofn.Flags = OFN_HIDEREADONLY | OFN_CREATEPROMPT | OFN_EXPLORER | OFN_OVERWRITEPROMPT;
    if (GetOpenFileNameA(&ofn) == TRUE) {
        HANDLE hf = CreateFileA(ofn.lpstrFile, GENERIC_ALL, 0, (LPSECURITY_ATTRIBUTES)NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL); //acces au fichier
    }
    else {
        PostQuitMessage(0x3f);
    }
}
void Splash()
{
    hWnd = GetConsoleWindow();
    InitCommonControls();
    CreateStatusWindow(WS_CHILD | WS_VISIBLE, "(C)opyright Patrice Waechter-Ebling 2022-2023", hWnd, 6000);
    SetConsoleTitle(titre);
    hMainMod = GetModuleHandle(0);
    ico = LoadIcon(hMainMod, (LPCSTR)0x65);
    SetConsoleIcon(ico);
}
void initDISM()
{
    HRESULT hr = S_OK;
    HRESULT hrLocal = S_OK;
    DismSession session = DISM_SESSION_DEFAULT;
    BOOL bMounted = FALSE;
    DWORD dwUnmountFlags = DISM_DISCARD_IMAGE;
    DismImageInfo* pImageInfo = NULL;
    DismPackage* pPackage = NULL;
    DismPackageInfo* pPackageInfo = NULL;
    DismFeature* pFeatures = NULL;
    UINT uiCount = 0;
}
int enumAgument(int argc,char** argv) {
    SetConsoleTitle(titre);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x4F | FOREGROUND_INTENSITY);
    for (int x = 0; x < argc; x++) {
        printf("Argument %d: %s\n", (x + 1), argv[x]);
    }
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0xC | FOREGROUND_INTENSITY);
    if (argc == 0x01) {
        ChargerImage();
        ExchangeColors(ofn.lpstrFile, FOREGROUND_GREEN,(char*)"Image choisie: " , FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    }
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_INTENSITY);

    return argc;

}
CHAR* GetDate() {
    GetLocalTime(&st);
    sprintf(date, "%4d%.2d%.2d", st.wYear, st.wMonth - 1, st.wDay);
    return date;
}
BOOL WINAPI SetConsoleIcon(HICON hIcon) {
    typedef BOOL(WINAPI* PSetConsoleIcon)(HICON);
    static PSetConsoleIcon pSetConsoleIcon = NULL;
    if (pSetConsoleIcon == NULL)	pSetConsoleIcon = (PSetConsoleIcon)GetProcAddress(GetModuleHandle("kernel32"), "SetConsoleIcon");
    if (pSetConsoleIcon == NULL)	return FALSE;
    return pSetConsoleIcon(hIcon);
}
DWORD GetVolumeInfo(LPCTSTR pDriveLetter, LPTSTR pDriveInfoBuffer, DWORD nDriveInfoSize) {
    DWORD serialNumber = 0;
    DWORD maxCompLength = 0;
    DWORD fileSysFlags = 0;
    DWORD lastError = 0;
    BOOL bGetVolInf = FALSE;
    UINT iDrvType = 0;
    typedef enum tagGetVolumeInfoResult { RESULTS_SUCCESS = 0, RESULTS_GETVOLUMEINFORMATION_FAILED = 1 };
    size_t size = sizeof(driveType) / sizeof(TCHAR);
    bGetVolInf = GetVolumeInformation(pDriveLetter, volumeName, sizeof(volumeName) / sizeof(TCHAR), &serialNumber, &maxCompLength, &fileSysFlags, fileSysName, sizeof(fileSysName) / sizeof(TCHAR));
    if (bGetVolInf == 0) {
        lastError = GetLastError();
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        _stprintf_s(pDriveInfoBuffer, nDriveInfoSize, TEXT(" ** Erreur %d lors de la requete d'information sur le lecteur %.1s **"), lastError, pDriveLetter);
        return RESULTS_GETVOLUMEINFORMATION_FAILED;
    }
    iDrvType = GetDriveType(pDriveLetter);
    switch (iDrvType) {
    case DRIVE_UNKNOWN:_stprintf_s(driveType, size, TEXT("%s"), TEXT("inconnu")); break;
    case DRIVE_NO_ROOT_DIR:_stprintf_s(driveType, size, TEXT("%s"), TEXT("Chemin?")); break;
    case DRIVE_REMOVABLE:_stprintf_s(driveType, size, TEXT("%s"), TEXT("disque dur externe")); break;
    case DRIVE_FIXED:_stprintf_s(driveType, size, TEXT("%s"), TEXT("disque dur interne")); break;
    case DRIVE_REMOTE:_stprintf_s(driveType, size, TEXT("%s"), TEXT("lecteur  reseau")); break;
    case DRIVE_CDROM:_stprintf_s(driveType, size, TEXT("%s"), TEXT("CD-ROM")); break;
    case DRIVE_RAMDISK:_stprintf_s(driveType, size, TEXT("%s"), TEXT("RAM-Disk")); break;
    default:_stprintf_s(driveType, size, TEXT("%s"), TEXT("indefini")); break;
    }
    if (_tcslen(volumeName) == 0) { _stprintf_s(volumeName, sizeof(volumeName) / sizeof(TCHAR), TEXT("%s"), TEXT("Nom indefini")); }
    ExchangeColors((CHAR*)volumeName, 0x02, (CHAR*)" ", 0x0);
    ExchangeColors((CHAR*)driveType, 0x03, (CHAR*)" ", 0x0);
    ExchangeColors((CHAR*)fileSysName, 0x04, (CHAR*)" ", 0x0);
    return RESULTS_SUCCESS;
}
VOID GetDiskSpaces(LPCTSTR pDriveLetter, LPTSTR pSpaceInfoBuffer, DWORD nSpaceInfoBufferSize) {
    unsigned __int64 i64TotalNumberOfBytes, i64TotalNumberOfFreeBytes, i64FreeBytesAvailableToCaller;
    BOOL bGetDiskFreeSpaceEx = FALSE;
    bGetDiskFreeSpaceEx = GetDiskFreeSpaceEx(pDriveLetter, (PULARGE_INTEGER)&i64FreeBytesAvailableToCaller, (PULARGE_INTEGER)&i64TotalNumberOfBytes, (PULARGE_INTEGER)&i64TotalNumberOfFreeBytes);
    if (bGetDiskFreeSpaceEx == TRUE) {
        _stprintf_s(pSpaceInfoBuffer, nSpaceInfoBufferSize, TEXT(" Disponibilite: %I64u/%I64uGo"), i64TotalNumberOfFreeBytes / (1024 * 1024 * 1024), i64TotalNumberOfBytes / (1024 * 1024 * 1024));
        i64NumberOfBytesUsed = (i64TotalNumberOfBytes - i64TotalNumberOfFreeBytes) / (static_cast<unsigned long long>(1024 * 1024) * 1024);
    }
}
CHAR* HarmoniseNom(CHAR* texte) {
    std::string str = texte;
    std::size_t found = str.find_first_of(" ");
    while (found != std::string::npos) { str[found] = '_'; found = str.find_first_of(" ", found + 1); }
    return(CHAR*)str.c_str();
}
CHAR* CreerDescription(CHAR* texte) {
    std::string str = texte;
    std::size_t found = str.find_first_of("_");
    while (found != std::string::npos) { str[found] = ' '; found = str.find_first_of("_", found + 1); }
    return(CHAR*)str.c_str();
}
INT MsgBox(CHAR* lpszText, CHAR* lpszCaption, DWORD dwStyle, INT lpszIcon) {
    MSGBOXPARAMS lpmbp;
    lpmbp.hInstance = hMainMod;
    lpmbp.cbSize = sizeof(MSGBOXPARAMS);
    lpmbp.hwndOwner = hWnd;
    lpmbp.dwLanguageId = MAKELANGID(0x0800, 0x0800); //par defaut celui du systeme
    lpmbp.lpszText = lpszText;
    if (lpszCaption != NULL) {
        lpmbp.lpszCaption = lpszCaption;
    }
    else {
        lpmbp.lpszCaption = titre;
    }
    lpmbp.dwStyle = dwStyle | 0x00000080L;
    if (lpszIcon == NULL) {
        lpmbp.lpszIcon = (LPCTSTR)0x65;
    }
    else {
        lpmbp.lpszIcon = (LPCTSTR)lpszIcon;
    }
    lpmbp.lpfnMsgBoxCallback = 0;
    return  MessageBoxIndirect(&lpmbp);
}
