package com.example.psrihariv.gentcmobile;

/**
 * Created by psrihariv on 3/1/2016.
 */
public class GenTCJNILib {

    static {

        System.loadLibrary("GenTCjni");
    }

    public static native void init(String path);
    public static native void draw();
    public static native void resize(int width, int height);

}
