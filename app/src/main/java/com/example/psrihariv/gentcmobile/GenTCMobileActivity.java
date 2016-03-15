package com.example.psrihariv.gentcmobile;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;

import java.io.File;

/**
 * Created by psrihariv on 3/1/2016.
 */
public class GenTCMobileActivity extends Activity {

    GenTCView mView;
    File folder;
    @Override protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        mView = new GenTCView(getApplication());

        folder = new File(Environment.getExternalStorageDirectory().toString()+"/GenTcAssets");
        Log.d(Environment.getExternalStorageDirectory().getPath(), "blah");
        if(folder.exists() && folder.isDirectory()) {
            //requestPermissions(new String[]{android.Manifest.permission.READ_EXTERNAL_STORAGE}, 123);
            Log.d(folder.getPath(),"Path found blah");
            File f[] = folder.listFiles();
            if(f!=null)
                Log.d("filename", f[0].getAbsolutePath());
        }
        else{

            Log.d("folder not found","blah");
        }
        mView.getpath(folder.getAbsolutePath());
        setContentView(mView);
    }

    @Override protected void onPause() {
        super.onPause();
        mView.onPause();
    }

    @Override protected void onResume() {
        super.onResume();
        mView.onResume();
    }

}
