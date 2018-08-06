package eu.sisik.cam

import android.app.Activity
import android.content.Context
import android.content.pm.PackageManager
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL
import android.hardware.camera2.CameraManager
import android.hardware.camera2.CameraMetadata.INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY
import android.os.Build
import android.os.Bundle
import android.widget.Toast
import kotlinx.android.synthetic.main.activity_main.*

class MainActivity : Activity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        if (!isNativeCamSupported()) {
            Toast.makeText(this, "Native camera probably won't work on this device!",
                    Toast.LENGTH_LONG).show()
            finish()
        }
    }

    override fun onResume() {
        super.onResume()

        // Ask for camera permission if necessary
        val camPermission = android.Manifest.permission.CAMERA
        if (Build.VERSION.SDK_INT >= 23 &&
                checkSelfPermission(camPermission) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(arrayOf(camPermission), CAM_PERMISSION_CODE)
        } else {
            initCam()
            camSurfaceView.onResume()
        }
    }

    override fun onPause() {
        super.onPause()
        camSurfaceView.onPause()
        exitCam()
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>?, grantResults: IntArray?) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == CAM_PERMISSION_CODE
                && grantResults?.get(0) != PackageManager.PERMISSION_GRANTED) {
            Toast.makeText(this, "This app requires camera permission", Toast.LENGTH_SHORT).show()
            finish()
        }
    }

    fun isNativeCamSupported(): Boolean {
        val camManager = getSystemService(Context.CAMERA_SERVICE) as CameraManager
        var nativeCamSupported = false

        for (camId in camManager.cameraIdList) {
            val characteristics = camManager.getCameraCharacteristics(camId)
            val hwLevel = characteristics.get(CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL)
            if (hwLevel != INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY) {
                nativeCamSupported = true
                break
            }
        }

        return nativeCamSupported
    }

    external fun initCam()
    external fun exitCam()

    companion object {
        val CAM_PERMISSION_CODE = 666

        init {
            System.loadLibrary("native-lib")
        }
    }
}
