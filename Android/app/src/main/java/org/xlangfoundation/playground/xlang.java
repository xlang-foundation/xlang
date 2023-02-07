package org.xlangfoundation.playground;
public class xlang {
    // Used to load the 'playground' library on application startup.
    static {
        System.loadLibrary("xlang");
    }
    private  MainActivity _activity;
    public  xlang(MainActivity activity)
    {
        _activity = activity;
    }
    public  void print(String info)
    {
        //_activity.Print(info);
    }
    public native boolean loadJNI();
    public native long loadModuleJNI(String code);
    public native boolean runJNI(String code);
    public native boolean unloadJNI();
}
