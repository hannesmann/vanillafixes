#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include <windows.h>

// Supported languages
typedef enum {
    LANG_ENGLISH,
    LANG_RUSSIAN,
    LANG_COUNT
} VF_LANGUAGE;

// String IDs
typedef enum {
    STR_CPU_NOT_SUPPORTED,
    STR_BUFFER_ERROR,
    STR_WOW_NOT_FOUND,
    STR_ADDITIONAL_DLLS,
    STR_VANILLAFIXES_TITLE,
    STR_PROCESS_ERROR,
    STR_DXVK_FAIL,
    STR_COUNT
} VF_STRING_ID;

// Initialize localization system
void LocalizationInit(void);

// Set specific language
void LocalizationSetLanguage(VF_LANGUAGE language);

// Get localized string by ID
LPCWSTR LocalizationGetString(VF_STRING_ID stringId);

#endif // LOCALIZATION_H
