#include <cstdio>
extern "C" const char *original_name();
extern "C" const char *original_path();
extern "C" const char *sdk_name();
int main(){auto p=original_name();std::printf("game=%02x%02x host=%02x sdk=%02x path=%s\n",(unsigned char)p[0],(unsigned char)p[1],(unsigned char)"日本"[0],(unsigned char)sdk_name()[0],original_path());}
