#include <windows.h>
#include <setupapi.h>

#include <jni.h>

#include "util.h"
#include "com_izforge_izpack_util_os_WinSetupAPIBase.h"

jobject g_jobj = NULL;
JavaVM *g_jvm;


#define DEBUG

#ifdef DEBUG
BOOL g_debug_console = false;
void init_debug_console()
{
	if (!g_debug_console) {
		AllocConsole();
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
		printf("(C) WinSetupAPI: HELLO WORLD\n");
		g_debug_console = true;
#ifdef _WIN32
		printf("_WIN32: true\n");
#else
		printf("_WIN32: not defined\n");
#endif
#ifdef _WIN64
		printf("_WIN64: true\n");
#else
		printf("_WIN64: not defined\n");
#endif
		printf("sizeof(size_t): %zu\n", sizeof(size_t));
		printf("sizeof(HSPFILEQ): %zu\n", sizeof(HSPFILEQ));
		printf("sizeof(jint): %zu\n", sizeof(jint));
		printf("sizeof(jlong): %zu\n", sizeof(jlong));
	}
}
#endif

// ----------------------------- Callback Helper functions ----------------------------

UINT MyQueueCallback_CopyError(PVOID pDefaultContext, UINT_PTR Param1, UINT_PTR Param2)
{
#ifdef DEBUG
	printf("(C) SPFILENOTIFY_COPYERROR callback...\n");
#endif
	// Attach thread to JVM
	JNIEnv* lpEnv;
	g_jvm->AttachCurrentThread((void**)&lpEnv, NULL);
	jclass cls = lpEnv->GetObjectClass(g_jobj);

	jmethodID jmID = lpEnv->GetMethodID(cls, "handleCopyError", "(Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;)I");
	jstring js1 = ((PFILEPATHS)Param1)->Source == NULL ? NULL : lpEnv->NewStringUTF(((PFILEPATHS)Param1)->Source);
	jstring js2 = ((PFILEPATHS)Param1)->Target == NULL ? NULL : lpEnv->NewStringUTF(((PFILEPATHS)Param1)->Target);
	jint win32err = ((PFILEPATHS)Param1)->Win32Error;
	jstring js3 = lpEnv->NewStringUTF(FormatSystemErrorMessage(((PFILEPATHS)Param1)->Win32Error, "SetupQueueCopy"));
	UINT ret = lpEnv->CallIntMethod(g_jobj, jmID, js1, js2, win32err, js3);
	return ret;
}

UINT MyQueueCallback_DeleteError(PVOID pDefaultContext, UINT_PTR Param1, UINT_PTR Param2)
{
#ifdef DEBUG
	printf("(C) SPFILENOTIFY_DELETEERROR callback...\n");
#endif
	// Attach thread to JVM
	JNIEnv* lpEnv;
	g_jvm->AttachCurrentThread((void**)&lpEnv, NULL);
	jclass cls = lpEnv->GetObjectClass(g_jobj);

	jmethodID jmID = lpEnv->GetMethodID(cls, "handleDeleteError", "(Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;)I");
	jstring js1 = ((PFILEPATHS)Param1)->Target == NULL ? NULL : lpEnv->NewStringUTF(((PFILEPATHS)Param1)->Target);
	jint win32err = ((PFILEPATHS)Param1)->Win32Error;
	jstring js3 = lpEnv->NewStringUTF(FormatSystemErrorMessage(((PFILEPATHS)Param1)->Win32Error, "SetupQueueDelete"));
	UINT ret = lpEnv->CallIntMethod(g_jobj, jmID, js1, win32err, js3);
	return ret;
}

UINT MyQueueCallback_RenameError(PVOID pDefaultContext, UINT_PTR Param1, UINT_PTR Param2)
{
#ifdef DEBUG
	printf("(C) SPFILENOTIFY_RENAMEERROR callback...\n");
#endif
	// Attach thread to JVM
	JNIEnv* lpEnv;
	g_jvm->AttachCurrentThread((void**)&lpEnv, NULL);
	jclass cls = lpEnv->GetObjectClass(g_jobj);

	jmethodID jmID = lpEnv->GetMethodID(cls, "handleRenameError", "(Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;)I");
	jstring js1 = ((PFILEPATHS)Param1)->Source == NULL ? NULL : lpEnv->NewStringUTF(((PFILEPATHS)Param1)->Source);
	jstring js2 = ((PFILEPATHS)Param1)->Target == NULL ? NULL : lpEnv->NewStringUTF(((PFILEPATHS)Param1)->Target);
	jint win32err = ((PFILEPATHS)Param1)->Win32Error;
	jstring js3 = lpEnv->NewStringUTF(FormatSystemErrorMessage(((PFILEPATHS)Param1)->Win32Error, "SetupQueueRename"));
	UINT ret = lpEnv->CallIntMethod(g_jobj, jmID, js1, js2, win32err, js3);
	return ret;
}

UINT MyQueueCallback_NeedMedia(PVOID pDefaultContext, UINT_PTR Param1, UINT_PTR Param2)
{
#ifdef DEBUG
	printf("(C) SPFILENOTIFY_NEEDMEDIA callback...\n");
#endif

	UINT ret;
	PSOURCE_MEDIA psm = (PSOURCE_MEDIA)Param1;

	LPTSTR path_str = (LPTSTR) LocalAlloc(LMEM_ZEROINIT,
		(strlen(psm->SourcePath) + strlen(psm->SourceFile)+ 2) * sizeof(TCHAR));
	lstrcpy(path_str, psm->SourcePath);
	lstrcat(path_str, "\\");
	lstrcat(path_str, psm->SourceFile);

#ifdef DEBUG
	printf("(C) SPFILENOTIFY_NEEDMEDIA: Checking for source file %s\n", path_str);
#endif

	OFSTRUCT ReOpenBuf = {};
	INT lzRet = LZOpenFile(path_str, &ReOpenBuf, OF_EXIST | OF_READ);

	if ((lzRet != LZERROR_BADINHANDLE) && (lzRet != LZERROR_GLOBALLOC))
	{
#ifdef DEBUG
		printf("(C) SPFILENOTIFY_NEEDMEDIA: Found %s\n", path_str);
#endif
		ret = SetupDefaultQueueCallback(pDefaultContext, SPFILENOTIFY_NEEDMEDIA, Param1, Param2);
	}
	else
	{
#ifdef DEBUG
		ErrorPrint("LZOpenFile");
		printf("(C) SPFILENOTIFY_NEEDMEDIA: callback to Java (%s)...\n", psm->SourceFile);
#endif
		// Attach thread to JVM
		JNIEnv* lpEnv;
		g_jvm->AttachCurrentThread((void**)&lpEnv, NULL);

		jclass cls = lpEnv->GetObjectClass(g_jobj);
		jmethodID jmID = lpEnv->GetMethodID(cls, "handleNeedMedia",
			"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I");

		jstring js1 = psm->Tagfile == NULL ? NULL : lpEnv->NewStringUTF(psm->Tagfile);
		jstring js2 = psm->Description == NULL ? NULL : lpEnv->NewStringUTF(psm->Description);
		jstring js3 = psm->SourcePath == NULL ? NULL : lpEnv->NewStringUTF(psm->SourcePath);
		jstring js4 = psm->SourceFile == NULL ? NULL : lpEnv->NewStringUTF(psm->SourceFile);

		ret = lpEnv->CallIntMethod(g_jobj, jmID, js1, js2, js3, js4);
	}

	LocalFree(path_str);
	return ret;
}

// ----------------------------- Main callback function ----------------------------

UINT WINAPI MyQueueCallback(
	PVOID pDefaultContext,
	UINT Notification,
	UINT_PTR Param1,
	UINT_PTR Param2)
{
#ifdef DEBUG
	printf("(C) MyQueueCallback...\n");
#endif

	UINT ret;

	switch (Notification)
	{
	case SPFILENOTIFY_COPYERROR:
		ret = MyQueueCallback_CopyError(pDefaultContext, Param1, Param2);
		break;
	case SPFILENOTIFY_DELETEERROR:
		ret = MyQueueCallback_DeleteError(pDefaultContext, Param1, Param2);
		break;
	case SPFILENOTIFY_RENAMEERROR:
		ret = MyQueueCallback_RenameError(pDefaultContext, Param1, Param2);
		break;
	case SPFILENOTIFY_NEEDMEDIA:
		ret = MyQueueCallback_NeedMedia(pDefaultContext, Param1, Param2);
		break;
	default:
		// Pass all other notifications through without modification
#ifdef DEBUG
		printf("(C) Unhandled notification %x, forwarding to standard handler\n", Notification);
#endif
		ret = SetupDefaultQueueCallback(pDefaultContext, Notification, Param1, Param2);
	}
#ifdef DEBUG
	printf("(C) Callback handler returns code %d\n", ret);
#endif
	return ret;
}

// ------------------- Implementation of native functions --------------------

JNIEXPORT jlong JNICALL Java_com_izforge_izpack_util_os_WinSetupAPIBase_SetupOpenFileQueue
(JNIEnv *env, jobject out, jobject msghandler)
{
  HSPFILEQ FileQueue;

  if (msghandler!=NULL)
  // Make object a Global Reference
  g_jobj = env->NewGlobalRef(msghandler);

  // Get Java VM
  env->GetJavaVM(&g_jvm);

#ifdef DEBUG
  init_debug_console();
  printf("(C) Opening new file queue...\n");
#endif
  FileQueue = SetupOpenFileQueue();

  if(!FileQueue || (FileQueue == INVALID_HANDLE_VALUE))
  {
#ifdef DEBUG
    ErrorPrint("SetupOpenFileQueue");
#endif
    ThrowIOExceptionLastError(env, "SetupOpenFileQueue");
  }

#ifdef DEBUG
  printf("(C) Opened file queue (%p).\n", FileQueue);
#endif
  return (jlong)FileQueue;
};

JNIEXPORT void JNICALL Java_com_izforge_izpack_util_os_WinSetupAPIBase_SetupCloseFileQueue
(JNIEnv *env, jobject out, jlong queuehandle)
{
	HSPFILEQ qhandle = (HSPFILEQ)queuehandle;

#ifdef DEBUG
  printf("(C) Closing file queue (%p)...\n", qhandle);
#endif
  SetupCloseFileQueue(qhandle);

  if (g_jobj!=NULL)
	env->DeleteGlobalRef(g_jobj);

  return;
};

JNIEXPORT void JNICALL Java_com_izforge_izpack_util_os_WinSetupAPIBase_SetupQueueCopy
(JNIEnv *env, jobject out, jlong queuehandle,
    jstring sourcerootpath, jstring sourcepath, jstring sourcefilename,
    jstring sourcedescription, jstring sourcetagfile,
    jstring targetdirectory, jstring targetfilename, jint copystyle)
{
  BOOL result;

  const char *str_sourcerootpath = sourcerootpath==NULL?NULL:env->GetStringUTFChars(sourcerootpath, 0);
  const char *str_sourcepath = sourcepath==NULL?NULL:env->GetStringUTFChars(sourcepath, 0);
  const char *str_sourcefilename = sourcefilename==NULL?NULL:env->GetStringUTFChars(sourcefilename, 0);
  const char *str_sourcedescription = sourcedescription==NULL?NULL:env->GetStringUTFChars(sourcedescription, 0);
  const char *str_sourcetagfile = sourcetagfile==NULL?NULL:env->GetStringUTFChars(sourcetagfile, 0);
  const char *str_targetdirectory = targetdirectory==NULL?NULL:env->GetStringUTFChars(targetdirectory, 0);
  const char *str_targetfilename = targetfilename==NULL?NULL:env->GetStringUTFChars(targetfilename, 0);

  result = SetupQueueCopy((HSPFILEQ)queuehandle,
      str_sourcerootpath, str_sourcepath, str_sourcefilename,
      str_sourcedescription, str_sourcetagfile,
      str_targetdirectory, str_targetfilename,
      (DWORD)copystyle);

  DWORD dw = GetLastError();

  env->ReleaseStringUTFChars(sourcerootpath, str_sourcerootpath);
  env->ReleaseStringUTFChars(sourcepath, str_sourcepath);
  env->ReleaseStringUTFChars(sourcefilename, str_sourcefilename);
  env->ReleaseStringUTFChars(sourcedescription, str_sourcedescription);
  env->ReleaseStringUTFChars(sourcetagfile, str_sourcetagfile);
  env->ReleaseStringUTFChars(targetdirectory, str_targetdirectory);
  env->ReleaseStringUTFChars(targetfilename, str_targetfilename);

  if(!result)
  {
#ifdef DEBUG
    ErrorPrint("SetupQueueCopy");
#endif
    ThrowIOExceptionSystemError(env, dw, "SetupQueueCopy");
  }

};

JNIEXPORT void JNICALL Java_com_izforge_izpack_util_os_WinSetupAPIBase_SetupQueueDelete
(JNIEnv *env, jobject out, jlong queuehandle, jstring pathpart1, jstring pathpart2)
{
  BOOL result;

  const char *str1 = pathpart1==NULL?NULL:env->GetStringUTFChars(pathpart1, 0);
  const char *str2 = pathpart2==NULL?NULL:env->GetStringUTFChars(pathpart2, 0);

  result = SetupQueueDelete((HSPFILEQ)queuehandle, str1, str2);

  DWORD dw = GetLastError();

  env->ReleaseStringUTFChars(pathpart1, str1);
  env->ReleaseStringUTFChars(pathpart2, str2);

  if(!result)
  {
#ifdef DEBUG
    ErrorPrint("SetupQueueDelete");
#endif
    ThrowIOExceptionSystemError(env, dw, "SetupQueueDelete");
  }
};

JNIEXPORT void JNICALL Java_com_izforge_izpack_util_os_WinSetupAPIBase_SetupQueueRename
(JNIEnv *env, jobject out, jlong queuehandle,
    jstring sourcepath, jstring sourcefilename,
    jstring targetpath, jstring targetfilename)
{
  BOOL result;

  const char *str_sourcepath = sourcepath==NULL?NULL:env->GetStringUTFChars(sourcepath, 0);
  const char *str_sourcefilename = sourcefilename==NULL?NULL:env->GetStringUTFChars(sourcefilename, 0);
  const char *str_targetpath = targetpath==NULL?NULL:env->GetStringUTFChars(targetpath, 0);
  const char *str_targetfilename = targetfilename==NULL?NULL:env->GetStringUTFChars(targetfilename, 0);

  result = SetupQueueRename((HSPFILEQ)queuehandle,
      str_sourcepath, str_sourcefilename,
      str_targetpath, str_targetfilename);

  DWORD dw = GetLastError();

  env->ReleaseStringUTFChars(sourcepath, str_sourcepath);
  env->ReleaseStringUTFChars(sourcefilename, str_sourcefilename);
  env->ReleaseStringUTFChars(targetpath, str_targetpath);
  env->ReleaseStringUTFChars(targetfilename, str_targetfilename);

  if(!result)
  {
#ifdef DEBUG
    ErrorPrint("SetupQueueRename");
#endif
    ThrowIOExceptionSystemError(env, dw, "SetupQueueRename");
  }
};

JNIEXPORT jboolean JNICALL Java_com_izforge_izpack_util_os_WinSetupAPIBase_SetupCommitFileQueue
(JNIEnv *env, jobject out, jlong queuehandle)
{
  PVOID lpCallbackContext = SetupInitDefaultQueueCallback(NULL);
  HSPFILEQ qhandle = (HSPFILEQ) queuehandle;

#ifdef DEBUG
  printf("(C) Committing file queue (%p)...\n", qhandle);
#endif

  PSP_FILE_CALLBACK msghandler;

  if (g_jobj!=NULL)
  {
    SetupDefaultQueueCallback(lpCallbackContext, 0, 0, 0);
    msghandler = (PSP_FILE_CALLBACK)MyQueueCallback;
  }
  else
  {
    msghandler = SetupDefaultQueueCallback;
  }

  BOOL result = SetupCommitFileQueue(NULL, qhandle, msghandler, lpCallbackContext);

  if(!result)
  {
#ifdef DEBUG
    ErrorPrint("SetupCommitFileQueue");
#endif
    ThrowIOExceptionLastError(env, "SetupCommitFileQueue");
  }

  SetupTermDefaultQueueCallback( lpCallbackContext );
  return result;
};

JNIEXPORT jint JNICALL Java_com_izforge_izpack_util_os_WinSetupAPIBase_SetupPromptReboot
(JNIEnv *env, jobject out, jlong queuehandle, jboolean scanonly)
{
  INT result;

  result = SetupPromptReboot((HSPFILEQ)queuehandle, NULL, scanonly);
  if (result == -1)
  {
#ifdef DEBUG
    ErrorPrint("SetupPromptReboot");
#endif
    ThrowIOExceptionLastError(env, "SetupPromptReboot");
  }
#ifdef DEBUG
  else
  if (scanonly)
  {
    printf("(C) SetupPromptReboot returned %x\n", result);
    if ((result & SPFILEQ_FILE_IN_USE) != 0)
    printf("(C) At least one file was in use during the queue commit process and there are delayed file operations pending.\n");
    if ((result & SPFILEQ_REBOOT_RECOMMENDED) != 0)
    printf("(C) The system should be rebooted.\n");
    if ((result & SPFILEQ_REBOOT_IN_PROGRESS) != 0)
    printf("(C) System shutdown is in progress.\n");
  }
#endif

  return result;
};
