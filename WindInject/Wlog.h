# pragma once

#include <Windows.h>
#include <stdio.h>

#define DP0(fmt) {char sOut[512];sprintf_s(sOut,512,fmt);Wputs(sOut);}    
#define DP1(fmt,var) {char sOut[512];sprintf_s(sOut,512,fmt,var);Wputs(sOut);}    
#define DP2(fmt,var1,var2) {char sOut[512];sprintf_s(sOut,512,fmt,var1,var2);Wputs(sOut);}    
#define DP3(fmt,var1,var2,var3) {char sOut[512];sprintf_s(sOut,512,fmt,var1,var2,var3);Wputs(sOut);}    
#define DP4(fmt,var1,var2,var3,var4) {char sOut[512];sprintf_s(sOut,512,fmt,var1,var2,var3,var4);Wputs(sOut);}    

void Wputs(const char *);