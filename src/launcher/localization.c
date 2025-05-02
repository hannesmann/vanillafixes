#include "localization.h"

// String tables for each language
static LPCWSTR g_EnglishStrings[STR_COUNT] = {
    [STR_CPU_NOT_SUPPORTED] = L"Your processor does not support the \"Invariant TSC\" feature.\r\n\r\nVanillaFixes can only work reliably on modern (2007+) Intel and AMD processors.",
    [STR_BUFFER_ERROR] = L"GetLastError() == ERROR_INSUFFICIENT_BUFFER",
    [STR_WOW_NOT_FOUND] = L"WoW.exe not found. Extract VanillaFixes into the same directory as the game.",
    [STR_ADDITIONAL_DLLS] = L"VanillaFixes will load additional DLLs listed in dlls.txt:\r\n%ls\r\n\r\nIf you don't agree, press Cancel. This reminder will be shown when the list of files has changed.",
    [STR_VANILLAFIXES_TITLE] = L"VanillaFixes",
    [STR_PROCESS_ERROR] = L"Error creating process: %ls\r\nThis issue can occur if you have enabled compatibility mode on the WoW executable.",
    [STR_DXVK_FAIL] = L"DXVK self-test failed (%ls). The game will now crash.\r\n\r\nThis is likely caused by missing Vulkan 1.3 support. The DXVK log file \"VanillaFixes_d3d9.log\" may have more information.\r\n\r\nIf you have an older GPU (R9 200/300, GTX 700), remove \"d3d9.dll\" to use VanillaFixes without DXVK. Otherwise, try updating your drivers to the latest version."
};

static LPCWSTR g_RussianStrings[STR_COUNT] = {
    [STR_CPU_NOT_SUPPORTED] = L"Ваш процессор не поддерживает функцию \"Invariant TSC\".\r\n\r\nVanillaFixes может стабильно работать только на современных (2007+) процессорах Intel и AMD.",
    [STR_BUFFER_ERROR] = L"GetLastError() == ERROR_INSUFFICIENT_BUFFER",
    [STR_WOW_NOT_FOUND] = L"WoW.exe не найден. Распакуйте VanillaFixes в папку с игрой.",
    [STR_ADDITIONAL_DLLS] = L"VanillaFixes загрузит дополнительные DLL, указанные в dlls.txt:\r\n%ls\r\n\r\nЕсли вы не согласны, нажмите Отмена. Это напоминание будет показано при изменении списка файлов.",
    [STR_VANILLAFIXES_TITLE] = L"VanillaFixes",
    [STR_PROCESS_ERROR] = L"Ошибка при создании процесса: %ls\r\nЭта проблема может возникнуть, если вы включили режим совместимости для исполняемого файла WoW.",
    [STR_DXVK_FAIL] = L"Самотестирование DXVK не удалось (%ls). Игра сейчас вылетит.\r\n\r\nСкорее всего, это вызвано отсутствием поддержки Vulkan 1.3. В файле журнала \"VanillaFixes_d3d9.log\" может быть больше информации.\r\n\r\nЕсли у вас старая видеокарта (R9 200/300, GTX 700), удалите \"d3d9.dll\", чтобы использовать VanillaFixes без DXVK. В противном случае, попробуйте обновить драйверы до последней версии."
};

// Array of all language string tables
static LPCWSTR* g_Languages[LANG_COUNT] = {
    [LANG_ENGLISH] = g_EnglishStrings,
    [LANG_RUSSIAN] = g_RussianStrings
};

// Current selected language
static VF_LANGUAGE g_CurrentLanguage = LANG_ENGLISH;

// Initialize localization based on system settings
void LocalizationInit(void) {
    LANGID langId = GetUserDefaultUILanguage();
    WORD primaryLangId = PRIMARYLANGID(langId);
    
    // Select language based on system locale
    switch (primaryLangId) {
        case LANG_RUSSIAN:
            g_CurrentLanguage = LANG_RUSSIAN;
            break;
        default:
            g_CurrentLanguage = LANG_ENGLISH;
            break;
    }
}

// Set specific language
void LocalizationSetLanguage(VF_LANGUAGE language) {
    if (language < LANG_COUNT) {
        g_CurrentLanguage = language;
    }
}

// Get localized string by ID
LPCWSTR LocalizationGetString(VF_STRING_ID stringId) {
    if (stringId < STR_COUNT) {
        return g_Languages[g_CurrentLanguage][stringId];
    }
    return L"";
}
