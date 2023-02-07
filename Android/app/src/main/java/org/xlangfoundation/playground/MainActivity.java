package org.xlangfoundation.playground;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;

import org.xlangfoundation.playground.databinding.ActivityMainBinding;
import org.xlangfoundation.playground.xlang;
public class MainActivity extends AppCompatActivity {

    private ActivityMainBinding binding;
    public void Print(String info)
    {
        TextView tv = binding.sampleText;
        tv.setText(info);
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        xlang xl = new xlang(this);
        boolean retVal = xl.loadJNI();
        String code =
                "import android\n"+
                        "def test(info):\n" +
                        "   android.print('from test func:${info}')\n" +
                        "test('first call')";

        xl.runJNI(code);
        //xl.unloadJNI();
    }
}