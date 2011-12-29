#if !defined(ANDROID_VERSION)
#error This file should only be included on the Android build
#endif

#include "acplatfm.h"
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h> 
#include <ctype.h>

#include <allegro.h>
#include "bigend.h"

#include <jni.h>
#include <android/log.h>


#define ANDROID_CONFIG_FILENAME "android.cfg"

extern char filetouse[];
char *INIreaditem(const char *sectn, const char *entry);
int INIreadint (const char *sectn, const char *item, int errornosect = 1);
bool ReadConfiguration(char* filename, bool read_everything);
void ResetConfiguration();

struct AGSAndroid : AGS32BitOSDriver {

  virtual int  CDPlayerCommand(int cmdd, int datt);
  virtual void Delay(int millis);
  virtual void DisplayAlert(const char*, ...);
  virtual unsigned long GetDiskFreeSpaceMB();
  virtual const char* GetNoMouseErrorString();
  virtual eScriptSystemOSID GetSystemOSID();
  virtual int  InitializeCDPlayer();
  virtual void PlayVideo(const char* name, int skip, int flags);
  virtual void PostAllegroExit();
  virtual int  RunSetup();
  virtual void SetGameWindowIcon();
  virtual void ShutdownCDPlayer();
  virtual void WriteConsole(const char*, ...);
  virtual void ReplaceSpecialPaths(const char *sourcePath, char *destPath);
  virtual void WriteDebugString(const char* texx, ...);
  virtual void ReadPluginsFromDisk(FILE *iii);
  virtual void StartPlugins();
  virtual void ShutdownPlugins();
  virtual int RunPluginHooks(int event, int data);
  virtual void RunPluginInitGfxHooks(const char *driverName, void *data);
  virtual int RunPluginDebugHooks(const char *scriptfile, int linenum);
};


//int psp_return_to_menu = 1;
int psp_ignore_acsetup_cfg_file = 0;
int psp_clear_cache_on_room_change = 0;
int psp_rotation = 0;
int psp_config_enabled = 0;


// Graphic options from the Allegro library.
extern int psp_gfx_scaling;
extern int psp_gfx_smoothing;


// Audio options from the Allegro library.
unsigned int psp_audio_samplerate = 44100;
int psp_audio_enabled = 1;
volatile int psp_audio_multithreaded = 1;
int psp_audio_cachesize = 10;
int psp_midi_enabled = 1;
int psp_midi_preload_patches = 0;

int psp_video_framedrop = 0;

int psp_gfx_renderer = 0;
int psp_gfx_super_sampling = 0;

extern int display_fps;
extern int want_exit;
extern void PauseGame();
extern void UnPauseGame();
extern int main(int argc,char*argv[]);

char psp_game_file_name[256];
char* psp_game_file_name_pointer = psp_game_file_name;

JNIEnv *java_environment;
jobject java_object;
jclass java_class;
jmethodID java_messageCallback;
jmethodID java_blockExecution;
jmethodID java_swapBuffers;
jmethodID java_setRotation;

bool reset_configuration = false;

extern "C" 
{

const int CONFIG_IGNORE_ACSETUP = 0;
const int CONFIG_CLEAR_CACHE = 1;
const int CONFIG_AUDIO_RATE = 2;
const int CONFIG_AUDIO_ENABLED = 3;
const int CONFIG_AUDIO_THREADED = 4;
const int CONFIG_AUDIO_CACHESIZE = 5;
const int CONFIG_MIDI_ENABLED = 6;
const int CONFIG_MIDI_PRELOAD = 7;
const int CONFIG_VIDEO_FRAMEDROP = 8;
const int CONFIG_GFX_RENDERER = 9;
const int CONFIG_GFX_SMOOTHING = 10;
const int CONFIG_GFX_SCALING = 11;
const int CONFIG_GFX_SS = 12;
const int CONFIG_ROTATION = 13;
const int CONFIG_ENABLED = 14;
const int CONFIG_DEBUG_FPS = 15;

extern void android_debug_printf(const char* format, ...);

JNIEXPORT jboolean JNICALL
  Java_com_bigbluecup_android_PreferencesActivity_readConfigFile(JNIEnv* env, jobject object, jstring directory)
{
  const char* cdirectory = env->GetStringUTFChars(directory, NULL);
  chdir(cdirectory);
  env->ReleaseStringUTFChars(directory, cdirectory);

  ResetConfiguration();

  return ReadConfiguration(ANDROID_CONFIG_FILENAME, true);
}


JNIEXPORT jboolean JNICALL
  Java_com_bigbluecup_android_PreferencesActivity_writeConfigFile(JNIEnv* env, jobject object)
{
  FILE* config = fopen(ANDROID_CONFIG_FILENAME, "wb");
  if (config)
  {
    fprintf(config, "[misc]\n");
    fprintf(config, "config_enabled = %d\n", psp_config_enabled);
    fprintf(config, "rotation = %d\n", psp_rotation);

    fprintf(config, "[compatibility]\n");
    fprintf(config, "ignore_acsetup_cfg_file = %d\n", psp_ignore_acsetup_cfg_file);
    fprintf(config, "clear_cache_on_room_change = %d\n", psp_clear_cache_on_room_change);

    fprintf(config, "[sound]\n");
    fprintf(config, "samplerate = %d\n", psp_audio_samplerate );
    fprintf(config, "enabled = %d\n", psp_audio_enabled);
    fprintf(config, "threaded = %d\n", psp_audio_multithreaded);
    fprintf(config, "cache_size = %d\n", psp_audio_cachesize);
    
    fprintf(config, "[midi]\n");
    fprintf(config, "enabled = %d\n", psp_midi_enabled);
    fprintf(config, "preload_patches = %d\n", psp_midi_preload_patches);

    fprintf(config, "[video]\n");
    fprintf(config, "framedrop = %d\n", psp_video_framedrop);

    fprintf(config, "[graphics]\n");
    fprintf(config, "renderer = %d\n", psp_gfx_renderer);
    fprintf(config, "smoothing = %d\n", psp_gfx_smoothing);
    fprintf(config, "scaling = %d\n", psp_gfx_scaling);
    fprintf(config, "super_sampling = %d\n", psp_gfx_super_sampling);

    fprintf(config, "[debug]\n");
    fprintf(config, "show_fps = %d\n", (display_fps == 2) ? 1 : 0);

    fclose(config);

    return true;
  }

  return false;
}


JNIEXPORT jint JNICALL
  Java_com_bigbluecup_android_PreferencesActivity_readIntConfigValue(JNIEnv* env, jobject object, jint id)
{
  switch (id)
  {
    case CONFIG_IGNORE_ACSETUP:
      return psp_ignore_acsetup_cfg_file;
      break;
    case CONFIG_CLEAR_CACHE:
      return psp_clear_cache_on_room_change;
      break;
    case CONFIG_AUDIO_RATE:
      return psp_audio_samplerate;
      break;
    case CONFIG_AUDIO_ENABLED:
      return psp_audio_enabled;
      break;
    case CONFIG_AUDIO_THREADED:
      return psp_audio_multithreaded;
      break;
    case CONFIG_AUDIO_CACHESIZE:
      return psp_audio_cachesize;
      break;
    case CONFIG_MIDI_ENABLED:
      return psp_midi_enabled;
      break;
    case CONFIG_MIDI_PRELOAD:
      return psp_midi_preload_patches;
      break;
    case CONFIG_VIDEO_FRAMEDROP:
      return psp_video_framedrop;
      break;
    case CONFIG_GFX_RENDERER:
      return psp_gfx_renderer;
      break;
    case CONFIG_GFX_SMOOTHING:
      return psp_gfx_smoothing;
      break;
    case CONFIG_GFX_SCALING:
      return psp_gfx_scaling;
      break;
    case CONFIG_GFX_SS:
      return psp_gfx_super_sampling;
      break;
    case CONFIG_ROTATION:
      return psp_rotation;
      break;
    case CONFIG_ENABLED:
      return psp_config_enabled;
      break;
    case CONFIG_DEBUG_FPS:
      return (display_fps == 2) ? 1 : 0;
      break;
    default:
      return 0;
      break;
  }
}


JNIEXPORT void JNICALL
  Java_com_bigbluecup_android_PreferencesActivity_setIntConfigValue(JNIEnv* env, jobject object, jint id, jint value)
{
  switch (id)
  {
    case CONFIG_IGNORE_ACSETUP:
      psp_ignore_acsetup_cfg_file = value;
      break;
    case CONFIG_CLEAR_CACHE:
      psp_clear_cache_on_room_change = value;
      break;
    case CONFIG_AUDIO_RATE:
      psp_audio_samplerate = value;
      break;
    case CONFIG_AUDIO_ENABLED:
      psp_audio_enabled = value;
      break;
    case CONFIG_AUDIO_THREADED:
      psp_audio_multithreaded = value;
      break;
    case CONFIG_AUDIO_CACHESIZE:
      psp_audio_cachesize = value;
      break;
    case CONFIG_MIDI_ENABLED:
      psp_midi_enabled = value;
      break;
    case CONFIG_MIDI_PRELOAD:
      psp_midi_preload_patches = value;
      break;
    case CONFIG_VIDEO_FRAMEDROP:
      psp_video_framedrop = value;
      break;
    case CONFIG_GFX_RENDERER:
      psp_gfx_renderer = value;
      break;
    case CONFIG_GFX_SMOOTHING:
      psp_gfx_smoothing = value;
      break;
    case CONFIG_GFX_SCALING:
      psp_gfx_scaling = value;
      break;
    case CONFIG_GFX_SS:
      psp_gfx_super_sampling = value;
      break;
    case CONFIG_ROTATION:
      psp_rotation = value;
      break;
    case CONFIG_ENABLED:
      psp_config_enabled = value;
      break;
    case CONFIG_DEBUG_FPS:
      display_fps = (value == 1) ? 2 : 0;
      break;
    default:
      break;
  }
}


JNIEXPORT void JNICALL
  Java_com_bigbluecup_android_EngineGlue_pauseEngine(JNIEnv* env, jobject object)
{
  PauseGame();
}

JNIEXPORT void JNICALL
  Java_com_bigbluecup_android_EngineGlue_resumeEngine(JNIEnv* env, jobject object)
{
  UnPauseGame();
}


JNIEXPORT void JNICALL
  Java_com_bigbluecup_android_EngineGlue_shutdownEngine(JNIEnv* env, jobject object)
{
  want_exit = 1;
}


JNIEXPORT jboolean JNICALL 
  Java_com_bigbluecup_android_EngineGlue_startEngine(JNIEnv* env, jobject object, jclass stringclass, jstring filename, jstring directory)
{
  // Get JNI interfaces.
  java_object = env->NewGlobalRef(object);
  java_environment = env;
  java_class = (jclass)java_environment->NewGlobalRef(java_environment->GetObjectClass(object));
  java_messageCallback = java_environment->GetMethodID(java_class, "showMessage", "(Ljava/lang/String;)V");
  java_blockExecution = java_environment->GetMethodID(java_class, "blockExecution", "()V");
  java_setRotation = java_environment->GetMethodID(java_class, "setRotation", "(I)V");

  // Initialize JNI for Allegro.
  android_allegro_initialize_jni(java_environment, java_class, java_object);

  // Get the file to run from Java.
  const char* cpath = java_environment->GetStringUTFChars(filename, NULL);
  strcpy(psp_game_file_name, cpath);
  java_environment->ReleaseStringUTFChars(filename, cpath);

  // Get the base directory (usually "/sdcard/ags").
  const char* cdirectory = java_environment->GetStringUTFChars(directory, NULL);
  chdir(cdirectory);
  java_environment->ReleaseStringUTFChars(directory, cdirectory);

  // Reset configuration.
  ResetConfiguration();

  // Read general configuration.
  ReadConfiguration(ANDROID_CONFIG_FILENAME, true);

	// Get the games path.
	char path[256];
	strcpy(path, psp_game_file_name);
	int lastindex = strlen(path) - 1;
	while (path[lastindex] != '/')
	{
	  path[lastindex] = 0;
	  lastindex--;
	}
  chdir(path);
  
  setenv("ULTRADIR", "..", 1);

  // Read game specific configuration.
  ReadConfiguration(ANDROID_CONFIG_FILENAME, false);

  // Set the screen rotation.
  if (psp_rotation > 0)
    java_environment->CallVoidMethod(java_object, java_setRotation, psp_rotation);

  // Start the engine main function.
	main(1, &psp_game_file_name_pointer);
  
  return true;
}

}



int ReadInteger(int* variable, char* section, char* name, int minimum, int maximum, int default_value)
{
  if (reset_configuration)
    return default_value;

  int temp = INIreadint(section, name);

  if (temp == -1)
    return 0;

  if ((temp < minimum) || (temp > maximum))
    temp = default_value;

  *variable = temp;

  return 1;
}



void ResetConfiguration()
{
  reset_configuration = true;

  ReadConfiguration(ANDROID_CONFIG_FILENAME, true);

  reset_configuration = false;
}



bool ReadConfiguration(char* filename, bool read_everything)
{
  FILE* test = fopen(filename, "rb");
  if (test || reset_configuration)
  {
    if (test)
      fclose(test);

    strcpy(filetouse, filename);

//    ReadInteger((int*)&psp_disable_powersaving, "misc", "disable_power_saving", 0, 1, 1);

//    ReadInteger((int*)&psp_return_to_menu, "misc", "return_to_menu", 0, 1, 1);

    ReadInteger((int*)&psp_config_enabled, "misc", "config_enabled", 0, 1, 0);
    if (!psp_config_enabled && !read_everything)
      return true;

    ReadInteger(&display_fps, "debug", "show_fps", 0, 1, 0);
    if (display_fps == 1)
      display_fps = 2;

    ReadInteger((int*)&psp_rotation, "misc", "rotation", 0, 2, 0);

    ReadInteger((int*)&psp_ignore_acsetup_cfg_file, "compatibility", "ignore_acsetup_cfg_file", 0, 1, 0);
    ReadInteger((int*)&psp_clear_cache_on_room_change, "compatibility", "clear_cache_on_room_change", 0, 1, 0);

    ReadInteger((int*)&psp_audio_samplerate, "sound", "samplerate", 0, 44100, 44100);
    ReadInteger((int*)&psp_audio_enabled, "sound", "enabled", 0, 1, 1);
    ReadInteger((int*)&psp_audio_multithreaded, "sound", "threaded", 0, 1, 1);

    ReadInteger((int*)&psp_midi_enabled, "midi", "enabled", 0, 1, 1);
    ReadInteger((int*)&psp_midi_preload_patches, "midi", "preload_patches", 0, 1, 0);

    int audio_cachesize;
    if (ReadInteger((int*)&audio_cachesize, "sound", "cache_size", 1, 50, 10));
      psp_audio_cachesize = audio_cachesize;

    ReadInteger((int*)&psp_video_framedrop, "video", "framedrop", 0, 1, 0);

    ReadInteger((int*)&psp_gfx_renderer, "graphics", "renderer", 0, 2, 0);
    ReadInteger((int*)&psp_gfx_smoothing, "graphics", "smoothing", 0, 1, 1);
    ReadInteger((int*)&psp_gfx_scaling, "graphics", "scaling", 0, 1, 1);
    ReadInteger((int*)&psp_gfx_super_sampling, "graphics", "super_sampling", 0, 1, 0);

    strcpy(filetouse, "nofile");

    return true;
  }

  return false;
}



void AGSAndroid::WriteDebugString(const char* texx, ...) {
  char displbuf[STD_BUFFER_SIZE] = "AGS: ";
  va_list ap;
  va_start(ap,texx);
  vsprintf(&displbuf[5],texx,ap);
  va_end(ap);
  __android_log_print(ANDROID_LOG_DEBUG, "AGSNative", displbuf);
}

void AGSAndroid::ReplaceSpecialPaths(const char *sourcePath, char *destPath)
{
  if (strnicmp(sourcePath, "$MYDOCS$", 8) == 0) 
  {
    strcpy(destPath, ".");
    strcat(destPath, &sourcePath[8]);
  }
  else if (strnicmp(sourcePath, "$APPDATADIR$", 12) == 0) 
  {
    strcpy(destPath, ".");
    strcat(destPath, &sourcePath[12]);
  }
  else {
    strcpy(destPath, sourcePath);
  }
}

int AGSAndroid::CDPlayerCommand(int cmdd, int datt) {
  return 1;//cd_player_control(cmdd, datt);
}

void AGSAndroid::DisplayAlert(const char *text, ...) {
  char displbuf[2000];
  va_list ap;
  va_start(ap, text);
  vsprintf(displbuf, text, ap);
  va_end(ap);
  __android_log_print(ANDROID_LOG_DEBUG, "AGSNative", displbuf);

  jstring java_string = java_environment->NewStringUTF(displbuf);
  java_environment->CallVoidMethod(java_object, java_messageCallback, java_string);
  java_environment->CallVoidMethod(java_object, java_blockExecution);
}

void AGSAndroid::Delay(int millis) {
  usleep(millis * 1000);
}

unsigned long AGSAndroid::GetDiskFreeSpaceMB() {
  // placeholder
  return 100;
}

const char* AGSAndroid::GetNoMouseErrorString() {
  return "This game requires a mouse. You need to configure and setup your mouse to play this game.\n";
}

eScriptSystemOSID AGSAndroid::GetSystemOSID() {
  return eOS_Win;
}

int AGSAndroid::InitializeCDPlayer() {
  return 1;//cd_player_init();
}

void AGSAndroid::PlayVideo(const char *name, int skip, int flags) {
  // do nothing
}

void AGSAndroid::PostAllegroExit() {
  java_environment->DeleteGlobalRef(java_class);
}

int AGSAndroid::RunSetup() {
  return 0;
}

void AGSAndroid::SetGameWindowIcon() {
  // do nothing
}

void AGSAndroid::WriteConsole(const char *text, ...) {
  char displbuf[2000];
  va_list ap;
  va_start(ap, text);
  vsprintf(displbuf, text, ap);
  va_end(ap);
  __android_log_print(ANDROID_LOG_DEBUG, "AGSNative", displbuf);  
}

void AGSAndroid::ShutdownCDPlayer() {
  //cd_exit();
}

void AGSAndroid::ReadPluginsFromDisk(FILE *iii) {
  pl_read_plugins_from_disk(iii);
}

void AGSAndroid::StartPlugins() {
  pl_startup_plugins();
}

void AGSAndroid::ShutdownPlugins() {
  pl_stop_plugins();
}

int AGSAndroid::RunPluginHooks(int event, int data) {
  return pl_run_plugin_hooks(event, data);
}

void AGSAndroid::RunPluginInitGfxHooks(const char *driverName, void *data) {
  pl_run_plugin_init_gfx_hooks(driverName, data);
}

int AGSAndroid::RunPluginDebugHooks(const char *scriptfile, int linenum) {
  return pl_run_plugin_debug_hooks(scriptfile, linenum);
}

AGSPlatformDriver* AGSPlatformDriver::GetDriver() {
  if (instance == NULL)
    instance = new AGSAndroid();

  return instance;
}