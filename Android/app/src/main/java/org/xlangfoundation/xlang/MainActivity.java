package org.xlangfoundation.xlang;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;

import org.xlangfoundation.xlang.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'xlang' library on application startup.
    static {
        System.loadLibrary("xlang");
    }

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Example of a call to a native method
        TextView tv = binding.sampleText;
        boolean retVal = loadJNI();
        runJNI("print('from java')");
        unloadJNI();
        tv.setText("loadJNI OK");
    }

    public native boolean loadJNI();
    public native boolean runJNI(String code);
    public native boolean unloadJNI();
}