#ifndef SIXO_LOG_H_
#define SIXO_LOG_H_

#include <android/log.h>

#define S(x) #x
#define S_(x) S(x)
#define S__LINE__ S_(__LINE__)


#if defined(NDEBUG)

#define LOGI(...) ((void)0)
#define LOGD(...) ((void)0)
#define LOGW(...) ((void)0)
#define LOGE(...) ((void)0)

#else

#define LOGI(...) (__android_log_print(ANDROID_LOG_SILENT, "LOGI: " __FILE__ " | " S__LINE__, __VA_ARGS__))
#define LOGD(...) (__android_log_print(ANDROID_LOG_DEBUG, "LOGD: " __FILE__ " | " S__LINE__, __VA_ARGS__))
#define LOGW(...) (__android_log_print(ANDROID_LOG_WARN, "LOGW: " __FILE__ " | " S__LINE__, __VA_ARGS__))
#define LOGE(...) (__android_log_print(ANDROID_LOG_ERROR, "LOGE: " __FILE__ " | " S__LINE__, __VA_ARGS__))

#endif
#endif