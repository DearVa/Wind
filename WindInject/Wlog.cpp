#include "Wlog.h"
#include "Hook.h"
#include "../WLink/Wlink.h"

void Wputs(const char *str) {
	SendWMessageC(0, strlen(str) + 1, (void *)str);
}
