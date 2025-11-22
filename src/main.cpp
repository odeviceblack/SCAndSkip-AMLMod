#include <mod/amlmod.h>
#include <logger.h>
#include <inttypes.h>
#include <jni.h>

MYMOD(com.deviceblack.gtasa.SCAndSkip, SCAndSkip, 1.3, DeviceBlack)

void* h_libSCAnd = nullptr;

void safeClear(JNIEnv* env)
{
	if(env->ExceptionCheck())
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
	}
}

void hideSystemUI()
{
	JNIEnv* env = aml->GetJNIEnvironment();
	if(!env) return logger->Error("invalid JNI environment!");

	jclass nvUtilClass = env->FindClass("com/nvidia/devtech/NvUtil");
	if(!nvUtilClass) return logger->Error("nvUtilClass not found!");
	safeClear(env);

	jmethodID getInstance = env->GetStaticMethodID(nvUtilClass, "getInstance", "()Lcom/nvidia/devtech/NvUtil;");
	if(!getInstance) return logger->Error("getInstance not found!");
	safeClear(env);

	jobject instanceObj = env->CallStaticObjectMethod(nvUtilClass, getInstance);
	if(!instanceObj) return logger->Error("instanceObj not found!");
	safeClear(env);

	jfieldID activityField = env->GetFieldID(nvUtilClass, "activity", "Landroid/app/Activity;");
	if(!activityField) return logger->Error("activityField not found!");
	safeClear(env);

	jobject activityObj = env->GetObjectField(instanceObj, activityField);
	if(!activityObj) return logger->Error("activityObj not found!");
	safeClear(env);

	jclass activityClass = env->FindClass("com/nvidia/devtech/NvEventQueueActivity");
	if(!activityClass) return logger->Error("activityClass not found!");
	safeClear(env);

	jmethodID hideSystemUI = env->GetMethodID(activityClass, "hideSystemUI", "()V");
	if(!hideSystemUI) return logger->Error("hideSystemUI not found!");
	safeClear(env);

	env->CallVoidMethod(activityObj, hideSystemUI);
	safeClear(env);

	env->DeleteLocalRef(nvUtilClass);
	env->DeleteLocalRef(instanceObj);
	env->DeleteLocalRef(activityObj);
	env->DeleteLocalRef(activityClass);
}

DECL_HOOKv(SocialClub_LoadScreen, void* self)
{
	hideSystemUI();

	void (*signInOffline)(void*) = nullptr;
	SETSYM_TO(signInOffline, h_libSCAnd, "_ZN10SocialClub13signInOfflineEv");

	if(signInOffline)
		signInOffline(self);
}

ON_MOD_LOAD()
{
	logger->SetTag("Test");

	h_libSCAnd = aml->GetLibHandle("libSCAnd.so");
	logger->Info("libSCAnd.so: 0x%" PRIXPTR, (uintptr_t)h_libSCAnd);

	HOOKSYM(SocialClub_LoadScreen, h_libSCAnd, "_ZN10SocialClub10LoadScreenEv");
}
