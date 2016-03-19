package com.example.psrihariv.gentcmobile;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.Log;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Created by psrihariv on 3/1/2016.
 */


public class GenTCView extends GLSurfaceView {

    private static final String TAG = "GenTcView";
    private static final boolean DEBUG = true;
    static String path;
    public GenTCView(Context context) {
        super(context);
        // Pick an EGLConfig with RGB8 color, 16-bit depth, no stencil,
        // supporting OpenGL ES 2.0 or later backwards-compatible versions.
        setEGLConfigChooser(8, 8, 8, 0, 16, 0);
        setEGLContextClientVersion(2);

        setRenderer(new Renderer());
    }
    public void getpath(String path){
        Log.d("got string", path);
        this.path = path;
    }
    private static class Renderer implements GLSurfaceView.Renderer {
        public void onDrawFrame(GL10 gl) {
            GenTCJNILib.draw();

        }

        public void onSurfaceChanged(GL10 gl, int width, int height) {
            GenTCJNILib.resize(width, height);
        }

        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            GenTCJNILib.init(path);
        }
    }
}
