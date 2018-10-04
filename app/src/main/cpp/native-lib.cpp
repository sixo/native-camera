#include <jni.h>
#include <string>
#include <fstream>
#include <vector>
#include <thread>

#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraMetadata.h>
#include <camera/NdkCameraDevice.h>
#include <media/NdkImageReader.h>
#include <android/native_window_jni.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "log.h"
#include "gl_helper.h"
#include "cam_utils.h"


using namespace sixo;


/**
 * Variables used to initialize and manage native camera
 */

static ACameraManager* cameraManager = nullptr;

static ACameraDevice* cameraDevice = nullptr;

static ACameraOutputTarget* textureTarget = nullptr;

static ACaptureRequest* textureRequest = nullptr;

static ANativeWindow* textureWindow = nullptr;

static ACameraCaptureSession* textureSession = nullptr;

static ACaptureSessionOutput* textureOutput = nullptr;

static ACaptureSessionOutput* output = nullptr;

static ACaptureSessionOutputContainer* outputs = nullptr;


/**
 * GL stuff - mostly used to draw the frames captured
 * by camera into a SurfaceTexture
 */

static GLuint prog;
static GLuint vtxShader;
static GLuint fragShader;

static GLint vtxPosAttrib;
static GLint uvsAttrib;
static GLint mvpMatrix;
static GLint texMatrix;
static GLint texSampler;
static GLint color;
static GLint size;
static GLuint buf[2];
static GLuint textureId;

static int width = 640;
static int height = 480;

static const char* vertexShaderSrc = R"(
        precision highp float;
        attribute vec3 vertexPosition;
        attribute vec2 uvs;
        varying vec2 varUvs;
        uniform mat4 texMatrix;
        uniform mat4 mvp;

        void main()
        {
            varUvs = (texMatrix * vec4(uvs.x, uvs.y, 0, 1.0)).xy;
            gl_Position = mvp * vec4(vertexPosition, 1.0);
        }
)";

static const char* fragmentShaderSrc = R"(
        #extension GL_OES_EGL_image_external : require
        precision mediump float;

        varying vec2 varUvs;
        uniform samplerExternalOES texSampler;
        uniform vec4 color;
        uniform vec2 size;

        void main()
        {
            if (gl_FragCoord.x/size.x < 0.5) {
                gl_FragColor = texture2D(texSampler, varUvs) * color;
            }
            else {
                const float threshold = 1.1;
                vec4 c = texture2D(texSampler, varUvs);
                if (length(c) > threshold) {
                    gl_FragColor = vec4(0.0, 0.0, 1.0, 1.0);
                } else {
                    gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
                }
            }
        }
)";


/**
 * Device listeners
 */

static void onDisconnected(void* context, ACameraDevice* device)
{
    LOGD("onDisconnected");
}

static void onError(void* context, ACameraDevice* device, int error)
{
    LOGD("error %d", error);
}

static ACameraDevice_stateCallbacks cameraDeviceCallbacks = {
        .context = nullptr,
        .onDisconnected = onDisconnected,
        .onError = onError,
};


/**
 * Session state callbacks
 */

static void onSessionActive(void* context, ACameraCaptureSession *session)
{
    LOGD("Session ready");
}

static void onSessionReady(void* context, ACameraCaptureSession *session)
{
    LOGD("Session ready");
}

static void onSessionClosed(void* context, ACameraCaptureSession *session)
{
    LOGD("Session closed");
}

static ACameraCaptureSession_stateCallbacks sessionStateCallbacks {
        .context = nullptr,
        .onActive = onSessionActive,
        .onReady = onSessionReady,
        .onClosed = onSessionClosed
};


/**
 * Capture callbacks
 */

void onCaptureFailed(void* context, ACameraCaptureSession* session,
                     ACaptureRequest* request, ACameraCaptureFailure* failure)
{
    LOGE("onCaptureFailed ");
}

void onCaptureSequenceCompleted(void* context, ACameraCaptureSession* session,
                                int sequenceId, int64_t frameNumber)
{}

void onCaptureSequenceAborted(void* context, ACameraCaptureSession* session,
                              int sequenceId)
{}

void onCaptureCompleted (
        void* context, ACameraCaptureSession* session,
        ACaptureRequest* request, const ACameraMetadata* result)
{
    LOGD("Capture completed");
}

static ACameraCaptureSession_captureCallbacks captureCallbacks {
        .context = nullptr,
        .onCaptureStarted = nullptr,
        .onCaptureProgressed = nullptr,
        .onCaptureCompleted = onCaptureCompleted,
        .onCaptureFailed = onCaptureFailed,
        .onCaptureSequenceCompleted = onCaptureSequenceCompleted,
        .onCaptureSequenceAborted = onCaptureSequenceAborted,
        .onCaptureBufferLost = nullptr,
};


/**
 * Functions used to set-up the camera and draw
 * camera frames into SurfaceTexture
 */

static void initCam()
{
    cameraManager = ACameraManager_create();

    auto id = getBackFacingCamId(cameraManager);
    ACameraManager_openCamera(cameraManager, id.c_str(), &cameraDeviceCallbacks, &cameraDevice);

    printCamProps(cameraManager, id.c_str());
}

static void exitCam()
{
    if (cameraManager)
    {
        // Stop recording to SurfaceTexture and do some cleanup
        ACameraCaptureSession_stopRepeating(textureSession);
        ACameraCaptureSession_close(textureSession);
        ACaptureSessionOutputContainer_free(outputs);
        ACaptureSessionOutput_free(output);

        ACameraDevice_close(cameraDevice);
        ACameraManager_delete(cameraManager);
        cameraManager = nullptr;

        // Capture request for SurfaceTexture
        ANativeWindow_release(textureWindow);
        ACaptureRequest_free(textureRequest);
    }
}

static void initSurface(JNIEnv* env, jint texId, jobject surface)
{
    // Init shaders
    vtxShader = createShader(vertexShaderSrc, GL_VERTEX_SHADER);
    fragShader = createShader(fragmentShaderSrc, GL_FRAGMENT_SHADER);
    prog = createProgram(vtxShader, fragShader);

    // Store attribute and uniform locations
    vtxPosAttrib = glGetAttribLocation(prog, "vertexPosition");
    uvsAttrib = glGetAttribLocation(prog, "uvs");
    mvpMatrix = glGetUniformLocation(prog, "mvp");
    texMatrix = glGetUniformLocation(prog, "texMatrix");
    texSampler = glGetUniformLocation(prog, "texSampler");
    color = glGetUniformLocation(prog, "color");
    size = glGetUniformLocation(prog, "size");

    // Prepare buffers
    glGenBuffers(2, buf);

    // Set up vertices
    float vertices[] {
            // x, y, z, u, v
            -1, -1, 0, 0, 0,
            -1, 1, 0, 0, 1,
            1, 1, 0, 1, 1,
            1, -1, 0, 1, 0
    };
    glBindBuffer(GL_ARRAY_BUFFER, buf[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    // Set up indices
    GLuint indices[] { 2, 1, 0, 0, 3, 2 };
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);


    // Camera target initialisation

    // Prepare surface
    textureId = texId;
    textureWindow = ANativeWindow_fromSurface(env, surface);

    // Prepare outputs for session
    ACaptureSessionOutput_create(textureWindow, &textureOutput);
    ACaptureSessionOutputContainer_create(&outputs);
    ACaptureSessionOutputContainer_add(outputs, textureOutput);

    // Create the session
    ACameraDevice_createCaptureSession(cameraDevice, outputs, &sessionStateCallbacks, &textureSession);

    // Prepare request for texture target
    ACameraDevice_createCaptureRequest(cameraDevice, TEMPLATE_PREVIEW, &textureRequest);

    ANativeWindow_acquire(textureWindow);
    ACameraOutputTarget_create(textureWindow, &textureTarget);
    ACaptureRequest_addTarget(textureRequest, textureTarget);

    // Start capturing continuously
    ACameraCaptureSession_setRepeatingRequest(textureSession, &captureCallbacks, 1, &textureRequest, nullptr);
}

static void drawFrame(JNIEnv* env, jfloatArray texMatArray)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glClearColor(0, 0, 0, 1);

    glUseProgram(prog);

    // Configure main transformations
    float mvp[] = {
            1.0f, 0, 0, 0,
            0, 1.0f, 0, 0,
            0, 0, 1.0f, 0,
            0, 0, 0, 1.0f
    };

    float aspect = width > height ? float(width)/float(height) : float(height)/float(width);
    if (width < height) // portrait
        ortho(mvp, -1.0f, 1.0f, -aspect, aspect, -1.0f, 1.0f);
    else // landscape
        ortho(mvp, -aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);

    glUniformMatrix4fv(mvpMatrix, 1, false, mvp);


    // Prepare texture for drawing

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Pass SurfaceTexture transformations to shader
    float* tm = env->GetFloatArrayElements(texMatArray, 0);
    glUniformMatrix4fv(texMatrix, 1, false, tm);
    env->ReleaseFloatArrayElements(texMatArray, tm, 0);

    // Set the SurfaceTexture sampler
    glUniform1i(texSampler, 0);

    // I use red color to mix with camera frames
    float c[] = { 1, 0, 0, 1 };
    glUniform4fv(color, 1, (GLfloat*)c);

    // Size of the window is used in fragment shader
    // to split the window
    float sz[2] = {0};
    sz[0] = width;
    sz[1] = height;
    glUniform2fv(size, 1, (GLfloat*)sz);

    // Set up vertices/indices and draw
    glBindBuffer(GL_ARRAY_BUFFER, buf[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf[1]);

    glEnableVertexAttribArray(vtxPosAttrib);
    glVertexAttribPointer(vtxPosAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)0);

    glEnableVertexAttribArray(uvsAttrib);
    glVertexAttribPointer(uvsAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void *)(3 * sizeof(float)));

    glViewport(0, 0, width, height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}


/**
 * JNI stuff
 */

extern "C" {

JNIEXPORT void JNICALL
Java_eu_sisik_cam_MainActivity_initCam(JNIEnv *env, jobject)
{
    initCam();
}

JNIEXPORT void JNICALL
Java_eu_sisik_cam_MainActivity_exitCam(JNIEnv *env, jobject)
{
    exitCam();
}

JNIEXPORT void JNICALL
Java_eu_sisik_cam_CamRenderer_onSurfaceCreated(JNIEnv *env, jobject, jint texId, jobject surface)
{
    initSurface(env, texId, surface);
}

JNIEXPORT void JNICALL
Java_eu_sisik_cam_CamRenderer_onSurfaceChanged(JNIEnv *env, jobject, jint w, jint h)
{
    width = w;
    height = h;
}

JNIEXPORT void JNICALL
Java_eu_sisik_cam_CamRenderer_onDrawFrame(JNIEnv *env, jobject, jfloatArray texMatArray)
{
    drawFrame(env, texMatArray);
}
} // Extern C