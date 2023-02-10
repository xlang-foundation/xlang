package org.xlangfoundation.playground;

import androidx.appcompat.app.AppCompatActivity;

public class xlang {
    // Used to load the 'playground' library on application startup.
    static {
        System.loadLibrary("xlang");
    }
    private AppCompatActivity _activity;
    public  xlang(AppCompatActivity activity)
    {
        _activity = activity;
    }
    public  void print(String info)
    {

    }
    public boolean createPage(String title)
    {
        ((MainActivity)_activity).CreateFragment();
        //return new Object();
        return true;
    }
    public native boolean loadJNI();
    public native long loadModuleJNI(String code);
    public native boolean runJNI(String code);
    public native boolean unloadJNI();
}
