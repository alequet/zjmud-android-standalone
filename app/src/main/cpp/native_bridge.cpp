#include <jni.h>
#include <android/log.h>
#include <event2/event.h>
#include <cstdio>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <vector>

extern int fluffos_main(int argc, char **argv);
extern void request_android_shutdown();

namespace {
void redirect_driver_output(const std::string &config_path) {
  const auto separator = config_path.find_last_of('/');
  if (separator == std::string::npos) return;

  const std::string log_path = config_path.substr(0, separator + 1) + "android-driver.log";
  const int log_fd = open(log_path.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0600);
  if (log_fd == -1) {
    __android_log_print(ANDROID_LOG_ERROR, "zjmud-native", "cannot open %s", log_path.c_str());
    return;
  }
  dup2(log_fd, STDOUT_FILENO);
  dup2(log_fd, STDERR_FILENO);
  close(log_fd);
  setvbuf(stdout, nullptr, _IONBF, 0);
  setvbuf(stderr, nullptr, _IONBF, 0);
}
}  // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_zjmud_android_NativeBridge_version(JNIEnv *env, jobject) {
#if defined(__aarch64__)
  constexpr const char *kAbi = "arm64-v8a";
#else
  constexpr const char *kAbi = "unsupported-abi";
#endif
  const std::string version = std::string("zjmud-native-smoke/") + kAbi;
  return env->NewStringUTF(version.c_str());
}

extern "C" JNIEXPORT jint JNICALL
Java_com_zjmud_android_NativeBridge_runDriver(JNIEnv *env, jobject, jstring config_path) {
  const char *path = env->GetStringUTFChars(config_path, nullptr);
  std::string config(path);
  env->ReleaseStringUTFChars(config_path, path);

  redirect_driver_output(config);
  const auto separator = config.find_last_of('/');
  if (separator != std::string::npos) {
    const std::string runtime_dir = config.substr(0, separator);
    if (chdir(runtime_dir.c_str()) == -1) {
      __android_log_print(ANDROID_LOG_ERROR, "zjmud-native", "cannot chdir to %s",
                          runtime_dir.c_str());
      return -1;
    }
  }

  std::vector<char> config_arg(config.begin(), config.end());
  config_arg.push_back('\0');
  char program[] = "zjmud";
  char *argv[] = {program, config_arg.data(), nullptr};
  __android_log_print(ANDROID_LOG_INFO, "zjmud-native", "starting driver: %s", config.c_str());
  return fluffos_main(2, argv);
}

extern "C" JNIEXPORT void JNICALL
Java_com_zjmud_android_NativeBridge_requestStop(JNIEnv *, jobject) {
  __android_log_print(ANDROID_LOG_INFO, "zjmud-native", "stop requested");
  request_android_shutdown();
}
