package eu.sisik.cam

import android.content.Context
import android.opengl.GLSurfaceView
import android.util.AttributeSet

class CamSurfaceView : GLSurfaceView {

    var camRenderer: CamRenderer

    constructor(context: Context?, attrs: AttributeSet) : super(context, attrs) {
        setEGLContextClientVersion(2)

        camRenderer = CamRenderer()
        setRenderer(camRenderer)
    }
}