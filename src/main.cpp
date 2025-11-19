#include <mod/amlmod.h>
#include <inttypes.h>
#include <logger.h>

MYMOD(SCAndSkip, SCAndSkip, 1.2, DeviceBlack)

//------------------------//
// Global variables
//------------------------//
void* h_SCAnd = nullptr;

//------------------------//
// Forward declarations
//------------------------//
void ExitSocialClub();
void SkipEULA();
void HookLoadScreen();
void EnableFullScreen();

//------------------------//
// Hook da função SocialClub::LoadScreen
//------------------------//
DECL_HOOKv(SocialClub_LoadScreen, void* self)
{
	// Chama a função original
	SocialClub_LoadScreen(self);
	EnableFullScreen();

	logger->Info("SocialClub_LoadScreen => Jumping to main menu!");

	JNIEnv* env = aml->GetJNIEnvironment();
	jclass gtasaClass = env->FindClass("com/rockstargames/gtasa/GTASA");
	if (!gtasaClass) return;

	jmethodID exitMethod = env->GetStaticMethodID(gtasaClass, "staticExitSocialClub", "()V");
	if (!exitMethod) return;

	env->CallStaticVoidMethod(gtasaClass, exitMethod);
	logger->Info("SocialClub_LoadScreen => Jumped successfully!");
}

//------------------------//
// Função para pular a tela do EULA
//------------------------//
void SkipEULA()
{
	logger->Info("Looking for LegalScreenShown symbol...");

	uintptr_t LegalScreenShown = aml->GetSym(h_SCAnd, "LegalScreenShown");
	if(!LegalScreenShown)
	{
		logger->Error("Failed to locate LegalScreenShown symbol!");
		return;
	}

	// Força a variável para true para pular o EULA
	*(bool*) LegalScreenShown = true;
	logger->Info("Successfully skipped Social Club EULA screen.");
}

//------------------------//
// Hook principal
//------------------------//
void HookLoadScreen()
{
	uintptr_t symLoadScreen = aml->GetSym(h_SCAnd, "_ZN10SocialClub10LoadScreenEv");
	if(!symLoadScreen)
	{
		logger->Error("Failed to locate SocialClub::LoadScreen symbol!");
		return;
	}

	HOOK(SocialClub_LoadScreen, symLoadScreen);
	logger->Info("Hooked SocialClub::LoadScreen successfully.");
}

//------------------------//
// Inicialização do mod
//------------------------//
void ExitSocialClub()
{
	// Carrega a biblioteca libSCAnd.so
	logger->Info("Loading libSCAnd.so...");
	h_SCAnd = aml->GetLibHandle("libSCAnd.so");

	if(!h_SCAnd)
	{
		logger->Error("Failed to load libSCAnd.so!");
		return;
	}

	logger->Info("Successfully loaded libSCAnd.so => 0x%" PRIXPTR, (uintptr_t) h_SCAnd);

	// Pula a tela do EULA
	SkipEULA();

	// Hook LoadScreen para pular login
	HookLoadScreen();
}

void EnableFullScreen()
{
	JNIEnv* env = aml->GetJNIEnvironment();

	// --- Obter classe GTASA ---
	jclass gtasaClass = env->FindClass("com/rockstargames/gtasa/GTASA");
	jfieldID selfField = env->GetStaticFieldID(gtasaClass, "gtasaSelf", "Lcom/rockstargames/gtasa/GTASA;");
	jobject activityObj = env->GetStaticObjectField(gtasaClass, selfField);
	jclass activityClass = env->GetObjectClass(activityObj);

	// getWindow()
	jmethodID getWindow = env->GetMethodID(activityClass, "getWindow", "()Landroid/view/Window;");
	jobject windowObj = env->CallObjectMethod(activityObj, getWindow);
	jclass windowClass = env->GetObjectClass(windowObj);

	if(android_get_device_api_level() < 12)
	{
		// getDecorView()
		jmethodID getDecorView = env->GetMethodID(windowClass, "getDecorView", "()Landroid/view/View;");
		jobject decorViewObj = env->CallObjectMethod(windowObj, getDecorView);
	
		// LEGACY FLAGS (ainda funcionam no Android 15)
		const int uiFlags =
			0x00000004 |  // FULLSCREEN
			0x00000400 |  // HIDE_NAVIGATION
			0x00001000 |  // IMMERSIVE_STICKY
			0x00000200;   // LAYOUT_HIDE_NAVIGATION
	
		jclass decorClass = env->GetObjectClass(decorViewObj);
		jmethodID setSystemUiVisibility = env->GetMethodID(decorClass, "setSystemUiVisibility", "(I)V");
		env->CallVoidMethod(decorViewObj, setSystemUiVisibility, uiFlags);
	}
	else
	{
		// --- ANDROID 12–15 MODE (WindowInsetsController) ---
		jmethodID getInsetsController = env->GetMethodID(windowClass, "getInsetsController", "()Landroid/view/WindowInsetsController;");
		jobject insetsController = env->CallObjectMethod(windowObj, getInsetsController);
	
		if (insetsController)
		{
			jclass insetsClass = env->GetObjectClass(insetsController);
	
			// hide(WindowInsets.Type.statusBars() | WindowInsets.Type.navigationBars())
			jclass windowInsetsType = env->FindClass("android/view/WindowInsets$Type");
			jmethodID statusBars = env->GetStaticMethodID(windowInsetsType, "statusBars", "()I");
			jmethodID navigationBars = env->GetStaticMethodID(windowInsetsType, "navigationBars", "()I");
	
			int types = env->CallStaticIntMethod(windowInsetsType, statusBars) | env->CallStaticIntMethod(windowInsetsType, navigationBars);
	
			jmethodID hide = env->GetMethodID(insetsClass, "hide", "(I)V");
			env->CallVoidMethod(insetsController, hide, types);
	
			// setSystemBarsBehavior(BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE)
			jmethodID setBehavior = env->GetMethodID(insetsClass, "setSystemBarsBehavior", "(I)V");

			int behavior = 2; // BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
			env->CallVoidMethod(insetsController, setBehavior, behavior);
		}
	}
	
	logger->Info("The statusBars and navigationBars have been hidden!");
}

//------------------------//
// Pré-carregamento do mod
//------------------------//
ON_MOD_PRELOAD()
{
	// Configurações do logger
	logger->SetTag(PROJECT_NAME_STR);
	
	/*
	logger->SetFile(aml->GetConfigPath(), "log_SCAndSkip.txt");
	logger->EnableFileLogging(true);
	logger->Info("File logging started!");
	*/

	// Inicializa o mod
	ExitSocialClub();
}
