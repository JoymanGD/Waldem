#pragma once

//from glfw3.h

/* The unknown key */
#define WD_KEY_UNKNOWN            -1

/* Printable keys */
#define WD_KEY_SPACE              32
#define WD_KEY_APOSTROPHE         39  /* ' */
#define WD_KEY_COMMA              44  /* , */
#define WD_KEY_MINUS              45  /* - */
#define WD_KEY_PERIOD             46  /* . */
#define WD_KEY_SLASH              47  /* / */
#define WD_KEY_0                  48
#define WD_KEY_1                  49
#define WD_KEY_2                  50
#define WD_KEY_3                  51
#define WD_KEY_4                  52
#define WD_KEY_5                  53
#define WD_KEY_6                  54
#define WD_KEY_7                  55
#define WD_KEY_8                  56
#define WD_KEY_9                  57
#define WD_KEY_SEMICOLON          59  /* ; */
#define WD_KEY_EQUAL              61  /* = */
#define WD_KEY_A                  65
#define WD_KEY_B                  66
#define WD_KEY_C                  67
#define WD_KEY_D                  68
#define WD_KEY_E                  69
#define WD_KEY_F                  70
#define WD_KEY_G                  71
#define WD_KEY_H                  72
#define WD_KEY_I                  73
#define WD_KEY_J                  74
#define WD_KEY_K                  75
#define WD_KEY_L                  76
#define WD_KEY_M                  77
#define WD_KEY_N                  78
#define WD_KEY_O                  79
#define WD_KEY_P                  80
#define WD_KEY_Q                  81
#define WD_KEY_R                  82
#define WD_KEY_S                  83
#define WD_KEY_T                  84
#define WD_KEY_U                  85
#define WD_KEY_V                  86
#define WD_KEY_W                  87
#define WD_KEY_X                  88
#define WD_KEY_Y                  89
#define WD_KEY_Z                  90
#define WD_KEY_LEFT_BRACKET       91  /* [ */
#define WD_KEY_BACKSLASH          92  /* \ */
#define WD_KEY_RIGHT_BRACKET      93  /* ] */
#define WD_KEY_GRAVE_ACCENT       96  /* ` */
#define WD_KEY_WORLD_1            161 /* non-US #1 */
#define WD_KEY_WORLD_2            162 /* non-US #2 */

/* Function keys */
#define WD_KEY_ESCAPE             256
#define WD_KEY_ENTER              257
#define WD_KEY_TAB                258
#define WD_KEY_BACKSPACE          259
#define WD_KEY_INSERT             260
#define WD_KEY_DELETE             261
#define WD_KEY_RIGHT              262
#define WD_KEY_LEFT               263
#define WD_KEY_DOWN               264
#define WD_KEY_UP                 265
#define WD_KEY_PAGE_UP            266
#define WD_KEY_PAGE_DOWN          267
#define WD_KEY_HOME               268
#define WD_KEY_END                269
#define WD_KEY_CAPS_LOCK          280
#define WD_KEY_SCROLL_LOCK        281
#define WD_KEY_NUM_LOCK           282
#define WD_KEY_PRINT_SCREEN       283
#define WD_KEY_PAUSE              284
#define WD_KEY_F1                 290
#define WD_KEY_F2                 291
#define WD_KEY_F3                 292
#define WD_KEY_F4                 293
#define WD_KEY_F5                 294
#define WD_KEY_F6                 295
#define WD_KEY_F7                 296
#define WD_KEY_F8                 297
#define WD_KEY_F9                 298
#define WD_KEY_F10                299
#define WD_KEY_F11                300
#define WD_KEY_F12                301
#define WD_KEY_F13                302
#define WD_KEY_F14                303
#define WD_KEY_F15                304
#define WD_KEY_F16                305
#define WD_KEY_F17                306
#define WD_KEY_F18                307
#define WD_KEY_F19                308
#define WD_KEY_F20                309
#define WD_KEY_F21                310
#define WD_KEY_F22                311
#define WD_KEY_F23                312
#define WD_KEY_F24                313
#define WD_KEY_F25                314
#define WD_KEY_KP_0               320
#define WD_KEY_KP_1               321
#define WD_KEY_KP_2               322
#define WD_KEY_KP_3               323
#define WD_KEY_KP_4               324
#define WD_KEY_KP_5               325
#define WD_KEY_KP_6               326
#define WD_KEY_KP_7               327
#define WD_KEY_KP_8               328
#define WD_KEY_KP_9               329
#define WD_KEY_KP_DECIMAL         330
#define WD_KEY_KP_DIVIDE          331
#define WD_KEY_KP_MULTIPLY        332
#define WD_KEY_KP_SUBTRACT        333
#define WD_KEY_KP_ADD             334
#define WD_KEY_KP_ENTER           335
#define WD_KEY_KP_EQUAL           336
#define WD_KEY_LEFT_SHIFT         340
#define WD_KEY_LEFT_CONTROL       341
#define WD_KEY_LEFT_ALT           342
#define WD_KEY_LEFT_SUPER         343
#define WD_KEY_RIGHT_SHIFT        344
#define WD_KEY_RIGHT_CONTROL      345
#define WD_KEY_RIGHT_ALT          346
#define WD_KEY_RIGHT_SUPER        347
#define WD_KEY_MENU               348

#define WD_KEY_LAST               WD_KEY_MENU