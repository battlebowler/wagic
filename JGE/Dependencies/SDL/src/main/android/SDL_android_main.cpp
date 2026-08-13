
/* Include the SDL main definition header */
#include "SDL_main.h"
#include "JGE.h"

#include "../../video/android/SDL_androidvideo.h"

/*******************************************************************************
                 Functions called by JNI
*******************************************************************************/
#include <jni.h>

// Called before SDL_main() to initialize JNI bindings in SDL library
extern "C" void SDL_Android_Init(JNIEnv* env, jclass cls);

// Library init
extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JGE *mEngine = JGE::GetInstance();
    mEngine->setJVM(vm);
    return JNI_VERSION_1_4;
}

// Start up the SDL app
extern "C" void Java_org_libsdl_app_SDLActivity_nativeInit(JNIEnv* env, jclass cls, jobject obj)
{
    /* This interface could expand with ABI negotiation, calbacks, etc. */
    SDL_Android_Init(env, cls);

    // Feed real device dimensions into JGE before the game starts
    JGE::SetScreenWidth(Android_ScreenWidth);
    JGE::SetScreenHeight(Android_ScreenHeight);

    /* Run the application code! */
    int status;
    char *argv[4];
    argv[0] = strdup("SDL_app");
	argv[1] = (char *)env;
    argv[2] = (char *)&cls;
    argv[3] = NULL;
    status = SDL_main(3, argv);

    /* We exit here for consistency with other platforms. */
    exit(status);
}

/* vi: set ts=4 sw=4 expandtab: */
