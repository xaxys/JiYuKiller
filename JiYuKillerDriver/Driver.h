#pragma once
#include <ntifs.h>

NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegPath);
VOID DriverUnload(_In_ struct _DRIVER_OBJECT *DriverObject);

NTSTATUS IOControlDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP Irp);
NTSTATUS CreateDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

VOID LoadFunctions();
VOID LoadUnExpFunctions();

NTSTATUS ZeroKill(ULONG_PTR PID);
NTSTATUS KillProcess(PEPROCESS pEThread);

VOID CompuleReBoot(void);
VOID CompuleShutdown(void);

//Defs

typedef struct _LDR_DATA_TABLE_ENTRY64
{
	LIST_ENTRY64    InLoadOrderLinks;
	LIST_ENTRY64    InMemoryOrderLinks;
	LIST_ENTRY64    InInitializationOrderLinks;
	PVOID            DllBase;
	PVOID            EntryPoint;
	ULONG            SizeOfImage;
	UNICODE_STRING    FullDllName;
	UNICODE_STRING     BaseDllName;
	ULONG            Flags;
	USHORT            LoadCount;
	USHORT            TlsIndex;
	PVOID            SectionPointer;
	ULONG            CheckSum;
	PVOID            LoadedImports;
	PVOID            EntryPointActivationContext;
	PVOID            PatchInformation;
	LIST_ENTRY64    ForwarderLinks;
	LIST_ENTRY64    ServiceTagLinks;
	LIST_ENTRY64    StaticLinks;
	PVOID            ContextInformation;
	ULONG64            OriginalBase;
	LARGE_INTEGER    LoadTime;
} LDR_DATA_TABLE_ENTRY64, *PLDR_DATA_TABLE_ENTRY64;

typedef struct _LDR_DATA_TABLE_ENTRY32 {
	LIST_ENTRY32 InLoadOrderLinks;
	LIST_ENTRY32 InMemoryOrderLinks;
	LIST_ENTRY32 InInitializationOrderLinks;
	ULONG DllBase;
	ULONG EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING32 FullDllName;
	UNICODE_STRING32 BaseDllName;
	ULONG Flags;
	USHORT LoadCount;
	USHORT TlsIndex;
	LIST_ENTRY32 HashLinks;
	ULONG SectionPointer;
	ULONG CheckSum;
	ULONG TimeDateStamp;
	ULONG LoadedImports;
	ULONG EntryPointActivationContext;
	ULONG PatchInformation;
	LIST_ENTRY32 ForwarderLinks;
	LIST_ENTRY32 ServiceTagLinks;
	LIST_ENTRY32 StaticLinks;
	ULONG ContextInformation;
	ULONG OriginalBase;
	LARGE_INTEGER LoadTime;
} LDR_DATA_TABLE_ENTRY32, *PLDR_DATA_TABLE_ENTRY32;

typedef enum _SHUTDOWN_ACTION {
	ShutdownNoReboot,
	ShutdownReboot,
	ShutdownPowerOff
} SHUTDOWN_ACTION, *PSHUTDOWN_ACTION;

typedef void* (__cdecl *memset_)(void*  _Dst, int _Val, size_t _Size);
typedef int(_stdcall *memcpy_)(void *Dst, const void *Src, size_t MaxCount);
typedef int(_stdcall *strcpy_)(char* _Destination, char const* _Source);
typedef int(_stdcall *strcat_)(char* _Destination, char const* _Source);
typedef int(_stdcall *swprintf_)(wchar_t* _Buffer, wchar_t const* _Format, ...);
typedef int(_stdcall *wcscpy_)(wchar_t* _Destination, _In_z_ wchar_t const* _Source);
typedef int(_stdcall *wcscat_)(wchar_t* _Destination, _In_z_ wchar_t const* _Source);

typedef NTSTATUS(_stdcall *PspTerminateThreadByPointer_)(IN PETHREAD Thread, IN NTSTATUS ExitStatus, IN BOOLEAN DirectTerminate);
typedef NTSTATUS(_stdcall *PspExitThread_)(IN NTSTATUS ExitStatus);
typedef NTSTATUS(_stdcall *PsResumeProcess_)(PEPROCESS Process);
typedef NTSTATUS(_stdcall *PsSuspendProcess_)(PEPROCESS Process);
typedef NTSTATUS(_stdcall *PsLookupProcessByProcessId_)(HANDLE ProcessId, PEPROCESS *Process);
typedef NTSTATUS(_stdcall *PsLookupThreadByThreadId_)(HANDLE ThreadId, PETHREAD *Thread);
typedef PETHREAD(_stdcall *PsGetNextProcessThread_)(IN PEPROCESS Process, IN PETHREAD Thread);
typedef NTSTATUS(_stdcall *PsTerminateProcess_)(PEPROCESS Process, NTSTATUS ExitStatus);
typedef PEPROCESS(_stdcall *PsGetNextProcess_)(PEPROCESS Process);
typedef ULONG(_stdcall *KeForceResumeThread_)(__inout PKTHREAD Thread);
typedef NTSTATUS(_stdcall *ZwTerminateProcess_)(_In_opt_ HANDLE ProcessHandle,_In_ NTSTATUS ExitStatus);
typedef NTSTATUS(_stdcall *ObRegisterCallbacks_)(_In_ POB_CALLBACK_REGISTRATION CallbackRegistration,	_Outptr_ PVOID *RegistrationHandle);
typedef VOID(_stdcall *ObUnRegisterCallbacks_)(	_In_ PVOID RegistrationHandle);
typedef USHORT(_stdcall *ObGetFilterVersion_)();
typedef NTSTATUS(_stdcall *PsSetCreateProcessNotifyRoutineEx_)(
	_In_ PCREATE_PROCESS_NOTIFY_ROUTINE_EX NotifyRoutine,
	_In_ BOOLEAN Remove
);
typedef NTSTATUS (NTAPI *NtShutdownSystem_)(IN SHUTDOWN_ACTION Action);





