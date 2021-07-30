/* Test strings */

#define W0 ""
#define W1 "1"
#define W7 "1234567"
#define W8 "12345678"
#define W32 W8 W8 W8 W8
#define W64 W32 W32
#define W255 W64 W64 W64 W32 W8 W8 W8 W7
#define W256 W64 W64 W64 W64
#define W257 W64 W64 W64 W32 W8 W8 W8 W8 W1
#define W1024 W256 W256 W256 W256
#define W4096 W1024 W1024 W1024 W1024

const char* w0 = W0;
const char* w1 = W1;
const char* w8 = W8;
const char* w32 = W32;
const char* w64 = W64;
const char* w255 = W64;
const char* w256 = W256;
const char* w257 = W257;
const char* w1024 = W1024;
const char* w4096 = W4096;
