#ifndef UCD_FILE_H
#define UCD_FILE_H

#if defined(_WIN32) || defined(_WIN64)
#    define UCD_PATH_SLASH "\\"
#else
#    define UCD_PATH_SLASH "/"
#endif

#define UCD_PATH       "data" UCD_PATH_SLASH
#define UCD_EMOJI_PATH UCD_PATH "emoji" UCD_PATH_SLASH
#define UCD_AUX_PATH   UCD_PATH "auxiliary" UCD_PATH_SLASH

#define UCD_UNICODE_DATA_PATH UCD_PATH "UnicodeData.txt"
#define UCD_DERIVED_NORMALIZATION_PROPS_PATH \
    UCD_PATH "DerivedNormalizationProps.txt"
#define UCD_PROP_LIST_PATH               UCD_PATH "PropList.txt"
#define UCD_DERIVED_CORE_PROPERTIES_PATH UCD_PATH "DerivedCoreProperties.txt"
#define UCD_EMOJI_DATA_PATH              UCD_EMOJI_PATH "emoji-data.txt"
#define UCD_GRAPHEME_BREAK_PROPERTY_PATH     \
    UCD_AUX_PATH "GraphemeBreakProperty.txt"
#define UCD_EAST_ASIAN_WIDTH_PATH    UCD_PATH "EastAsianWidth.txt"
#define UCD_GRAPHEME_BREAK_TEST_PATH UCD_AUX_PATH "GraphemeBreakTest.txt"

const char *file_load(const char *path);
void file_free(const char *file);

#endif
