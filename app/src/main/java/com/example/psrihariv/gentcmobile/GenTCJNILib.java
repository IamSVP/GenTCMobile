package com.example.psrihariv.gentcmobile;

/**
 * Created by psrihariv on 3/1/2016.
 */
public class GenTCJNILib {

    static {

        System.loadLibrary("GenTCjni");
    }

    public static native void init();
    public static native void Render();

}
