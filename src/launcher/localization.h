#pragma execution_character_set("utf-8")
#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include <windows.h>

// Supported languages
enum {
    VF_LANG_ENGLISH = 0,
    VF_LANG_RUSSIAN,
    VF_LANG_COUNT
};

// String IDs
enum {
    STR_CPU_NOT_SUPPORTED = 0,
    STR_BUFFER_ERROR,
    STR_WOW_NOT_FOUND,
    STR_ADDITIONAL_DLLS,
    STR_VANILLAFIXES_TITLE,
    STR_PROCESS_ERROR,
    STR_DXVK_FAIL,
    STR_COUNT
};

// Initialize localization system
void LocalizationInit(void);

// Set specific language
void LocalizationSetLanguage(int language);

// Get localized string by ID
LPCWSTR LocalizationGetString(int stringId);

#endif // LOCALIZATION_H
