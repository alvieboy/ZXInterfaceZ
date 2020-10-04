#ifdef __cplusplus
extern "C" {
#endif

extern const uint8_t CHAR_SET[];
extern const uint8_t RIGHTVERTICAL[8];
extern const uint8_t LEFTVERTICAL[8];
extern const uint8_t HH[8];
extern const uint8_t UPARROW[8];
extern const uint8_t DOWNARROW[8];
extern const uint8_t CENTERVERTICAL[8];

#define NUM_PRINTABLE_CHARS 96
#define FIRST_PRINTABLE_CHAR 32

#define IS_PRINTABLE(c) ((c)>=FIRST_PRINTABLE_CHAR && ((c)<(FIRST_PRINTABLE_CHAR+NUM_PRINTABLE_CHARS)))

#ifdef __cplusplus
}
#endif
