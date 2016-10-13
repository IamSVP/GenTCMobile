package com.example.psrihariv.gentcmobile;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.Log;
import android.view.MotionEvent;

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
        m_AngleX = 0.0f;
        m_AngleY = 0.0f;
        setRenderer(new Renderer(this));

    }
    public void getpath(String path){
        Log.d("got string", path);
        this.path = path;
    }
    public void invalidateFrame(){
        this.invalidate();
    }
    @Override
    public boolean onTouchEvent(MotionEvent e){

        float x = e.getX();
        float y = e.getY();
       // Log.d("positions: ", "X:" +x + "--Y:" +y);
        switch(e.getAction()){
            case MotionEvent.ACTION_MOVE:

                float dx = x - m_PreviousX;
                float dy = y - m_PreviousY;

//                if(y > getHeight() / 2){
//                    dx = dx * -1;
//                }
//
//                if(x < getWidth() /2){
//                    dy = dy * -1;
//                }
                m_AngleX += (dx) * TOUCH_SCALE_FACTOR;
                m_AngleY += (dy) * TOUCH_SCALE_FACTOR;
                m_AngleX %= 360;
                m_AngleY %= 360;
                break;


        }

        m_PreviousX = x;
        m_PreviousY = y;
        return true;
    }

    float TOUCH_SCALE_FACTOR = 180.0f/1000;
    float m_PreviousX;
    float m_PreviousY;
    static float m_AngleX;
    static float m_AngleY;

    private static class Renderer implements GLSurfaceView.Renderer {
        GLSurfaceView p;
        public Renderer(GLSurfaceView parent){
            p = parent;
        }
        public void onDrawFrame(GL10 gl) {

            GenTCJNILib.draw(m_AngleX, m_AngleY);
            p.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
            //p.invalidate

        }

        public void onSurfaceChanged(GL10 gl, int width, int height) {
            GenTCJNILib.resize(width, height);
        }

        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            GenTCJNILib.init(path);
        }
    }
}
