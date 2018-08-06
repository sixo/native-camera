package eu.sisik.cam

import android.graphics.SurfaceTexture
import android.opengl.GLES11Ext.GL_TEXTURE_EXTERNAL_OES
import android.opengl.GLES20
import android.opengl.GLSurfaceView
import android.util.Log
import android.view.Surface
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10


class CamRenderer: GLSurfaceView.Renderer {

    lateinit var surfaceTexture: SurfaceTexture
    val texMatrix = FloatArray(16)
    @Volatile var frameAvailable: Boolean = false
    val lock = Object()

    override fun onDrawFrame(p0: GL10?) {
        synchronized(lock) {
            if (frameAvailable) {
                Log.d("sixo", "Frame available...updating")
                surfaceTexture.updateTexImage()
                surfaceTexture.getTransformMatrix(texMatrix)
                frameAvailable = false
            }
        }

        onDrawFrame(texMatrix)
    }

    override fun onSurfaceChanged(p0: GL10?, width: Int, height: Int) {
        onSurfaceChanged(width, height)
    }

    override fun onSurfaceCreated(p0: GL10?, p1: EGLConfig?) {
        // Prepare texture and surface
        val textures = IntArray(1)
        GLES20.glGenTextures(1, textures, 0)
        GLES20.glBindTexture(GL_TEXTURE_EXTERNAL_OES, textures[0])


        surfaceTexture = SurfaceTexture(textures[0])
        surfaceTexture.setOnFrameAvailableListener {
            synchronized(lock) {
                frameAvailable = true
            }
        }

        val surface = Surface(surfaceTexture)

        // Pass to native code
        onSurfaceCreated(textures[0], surface)
    }

    external fun onSurfaceCreated(textureId: Int, surface: Surface)
    external fun onSurfaceChanged(width: Int, height: Int)
    external fun onDrawFrame(texMat: FloatArray)
}