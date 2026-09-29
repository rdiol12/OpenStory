/* Java runtime hardware probe, not the Cosmic server. */
#include <jni.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

extern int sceNetInit(void);
extern int sceNetPoolCreate(const char *, int, int);

int main(void) {
    if (mkdir("/download0/cosmic", 0700) != 0 && errno != EEXIST) return 1;
    int log = open("/download0/cosmic/java-probe.log", O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (log < 0) return 1;
    if (dup2(log, STDOUT_FILENO) < 0 || dup2(log, STDERR_FILENO) < 0) { close(log); return 1; }
    close(log);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    puts("Cosmic PS5 Java runtime probe; this is not the server.");
    if (mkdir("/download0/cosmic/tmp", 0700) != 0 && errno != EEXIST) { perror("mkdir"); return 1; }
    if (setenv("JAVA_HOME", "/app0/java", 1) != 0) { perror("setenv"); return 1; }
    int network = sceNetInit();
    if (network < 0) { fprintf(stderr, "sceNetInit failed: %#x\n", network); return 1; }
    int pool = sceNetPoolCreate("CosmicJavaProbe", 4 * 1024 * 1024, 0);
    if (pool < 0) { fprintf(stderr, "sceNetPoolCreate failed: %#x\n", pool); return 1; }

    JavaVMOption options[] = {
        {.optionString = "-Xms32m"}, {.optionString = "-Xmx256m"},
        {.optionString = "-XX:+UseSerialGC"}, {.optionString = "-Xrs"},
        {.optionString = "-Djava.class.path=/app0/probe"},
        {.optionString = "-Djava.io.tmpdir=/download0/cosmic/tmp"},
        {.optionString = "-XX:ErrorFile=/download0/cosmic/java-error.log"}
    };
    JavaVMInitArgs args = {.version = JNI_VERSION_1_8,
        .nOptions = sizeof(options) / sizeof(options[0]), .options = options, .ignoreUnrecognized = JNI_FALSE};
    JavaVM *vm = NULL;
    JNIEnv *env = NULL;
    puts("Starting the embedded Java 21 VM...");
    int result = JNI_CreateJavaVM(&vm, (void **)&env, &args);
    if (result != JNI_OK) { fprintf(stderr, "JNI_CreateJavaVM failed: %d\n", result); return 1; }
    jclass probe = (*env)->FindClass(env, "RuntimeProbe");
    jmethodID run = probe ? (*env)->GetStaticMethodID(env, probe, "run", "()V") : NULL;
    if (run) (*env)->CallStaticVoidMethod(env, probe, run);
    int failed = (*env)->ExceptionCheck(env) || run == NULL;
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionDescribe(env);
    if (failed) fputs("FAIL: Java runtime probe\n", stderr);
    else puts("PASS: Java runtime probe");
    result = (*vm)->DestroyJavaVM(vm);
    if (result != JNI_OK) fprintf(stderr, "DestroyJavaVM failed: %d\n", result);
    return failed || result != JNI_OK;
}
