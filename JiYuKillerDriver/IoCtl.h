#pragma once

#define CTL_INITPARAM CTL_CODE(FILE_DEVICE_UNKNOWN,0x989D,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define CTL_INITSELFPROTECT CTL_CODE(FILE_DEVICE_UNKNOWN,0x989E,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define CTL_OPEN_PROCESS CTL_CODE(FILE_DEVICE_UNKNOWN,0x89F,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define CTL_KILL_PROCESS CTL_CODE(FILE_DEVICE_UNKNOWN,0x900,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define CTL_OPEN_THREAD CTL_CODE(FILE_DEVICE_UNKNOWN,0x901,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define CTL_KILL_THREAD CTL_CODE(FILE_DEVICE_UNKNOWN,0x902,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define CTL_KILL_PROCESS_SPARE_NO_EFFORT CTL_CODE(FILE_DEVICE_UNKNOWN,0x903,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define CTL_TREMINATE_PROCESS CTL_CODE(FILE_DEVICE_UNKNOWN,0x906,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define CTL_TREMINATE_THREAD CTL_CODE(FILE_DEVICE_UNKNOWN,0x907,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define CTL_SUSPEND_PROCESS CTL_CODE(FILE_DEVICE_UNKNOWN,0x908,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define CTL_RESUME_PROCESS CTL_CODE(FILE_DEVICE_UNKNOWN,0x909,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define CTL_SHUTDOWN CTL_CODE(FILE_DEVICE_UNKNOWN,0x90A,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define CTL_REBOOT CTL_CODE(FILE_DEVICE_UNKNOWN,0x90B,METHOD_BUFFERED,FILE_ANY_ACCESS)


