17 stub InitErrors
18 stub PostError
19 stub InitSSAutoEnterThread
20 stub UpdateError
22 stdcall LoadStringRC(long ptr long long)
23 stub ReOpenMetaDataWithMemory

@ stub CallFunctionShim
@ stub CloseCtrs
@ stub ClrCreateManagedInstance
@ stub CoEEShutDownCOM
@ stdcall CoInitializeCor(long)
@ stub CoInitializeEE
@ stub CoUninitializeCor
@ stub CoUninitializeEE
@ stub CollectCtrs
@ stub CorBindToCurrentRuntime
@ stub CorBindToRuntime
@ stub CorBindToRuntimeByCfg
@ stub CorBindToRuntimeByPath
@ stub CorBindToRuntimeByPathEx
@ stub CorBindToRuntimeEx
@ stdcall CorBindToRuntimeHost(wstr wstr wstr ptr long ptr ptr ptr)
@ stub CorDllMainWorker
@ stdcall CorExitProcess(long)
@ stub CorGetSvc
@ stub CorIsLatestSvc
@ stub CorMarkThreadInThreadPool
@ stub CorTickleSvc
@ stub CreateConfigStream
@ stub CreateDebuggingInterfaceFromVersion
@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stub DllRegisterServer
@ stub DllUnregisterServer
@ stub EEDllGetClassObjectFromClass
@ stub EEDllRegisterServer
@ stub EEDllUnregisterServer
@ stdcall GetAssemblyMDImport(ptr ptr ptr)
@ stub GetCORRequiredVersion
@ stub GetCORRootDirectory
@ stdcall GetCORSystemDirectory(ptr long ptr)
@ stdcall GetCORVersion(ptr long ptr)
@ stub GetCompileInfo
@ stub GetFileVersion
@ stub GetHashFromAssemblyFile
@ stub GetHashFromAssemblyFileW
@ stub GetHashFromBlob
@ stub GetHashFromFile
@ stub GetHashFromFileW
@ stub GetHashFromHandle
@ stub GetHostConfigurationFile
@ stub GetMetaDataInternalInterface
@ stub GetMetaDataInternalInterfaceFromPublic
@ stub GetMetaDataPublicInterfaceFromInternal
@ stub GetPermissionRequests
@ stub GetPrivateContextsPerfCounters
@ stub GetProcessExecutableHeap
@ stub GetRealProcAddress
@ stdcall GetRequestedRuntimeInfo(wstr wstr wstr long long ptr long ptr ptr long ptr)
@ stub GetRequestedRuntimeVersion
@ stub GetRequestedRuntimeVersionForCLSID
@ stub GetStartupFlags
@ stub GetTargetForVTableEntry
@ stub GetTokenForVTableEntry
@ stdcall GetVersionFromProcess(ptr ptr long ptr)
@ stub GetXMLElement
@ stub GetXMLElementAttribute
@ stub GetXMLObject
@ stdcall LoadLibraryShim(ptr ptr ptr ptr)
@ stub LoadLibraryWithPolicyShim
@ stdcall LoadStringRCEx(long long ptr long long ptr)
@ stub LockClrVersion
@ stub MetaDataGetDispenser
@ stub OpenCtrs
@ stub ReOpenMetaDataWithMemoryEx
@ stub RunDll@ShimW
@ stub RuntimeOSHandle
@ stub RuntimeOpenImage
@ stub RuntimeReleaseHandle
@ stub SetTargetForVTableEntry
@ stub StrongNameCompareAssemblies
@ stub StrongNameErrorInfo
@ stub StrongNameFreeBuffer
@ stub StrongNameGetBlob
@ stub StrongNameGetBlobFromImage
@ stub StrongNameGetPublicKey
@ stub StrongNameHashSize
@ stub StrongNameKeyDelete
@ stub StrongNameKeyGen
@ stub StrongNameKeyGenEx
@ stub StrongNameKeyInstall
@ stub StrongNameSignatureGeneration
@ stub StrongNameSignatureGenerationEx
@ stub StrongNameSignatureSize
@ stub StrongNameSignatureVerification
@ stub StrongNameSignatureVerificationEx
@ stub StrongNameSignatureVerificationFromImage
@ stub StrongNameTokenFromAssembly
@ stub StrongNameTokenFromAssemblyEx
@ stub StrongNameTokenFromPublicKey
@ stub TranslateSecurityAttributes
@ stdcall _CorDllMain(long long ptr)
@ stdcall _CorExeMain2(ptr long ptr ptr ptr)
@ stdcall _CorExeMain()
@ stdcall _CorImageUnloading(ptr)
@ stdcall _CorValidateImage(ptr ptr)
