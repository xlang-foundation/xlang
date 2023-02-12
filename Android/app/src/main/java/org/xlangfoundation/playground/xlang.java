package org.xlangfoundation.playground;

import android.app.Fragment;
import android.graphics.Color;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.constraintlayout.widget.ConstraintLayout;

import org.w3c.dom.Text;

import java.util.ArrayList;
import java.util.List;

public class xlang {
    // Used to load the 'playground' library on application startup.
    static {
        System.loadLibrary("xlang");
    }
    private AppCompatActivity _activity;
    private View _curPage;
    private  long _curModuleKey =0;
    public  xlang(AppCompatActivity activity)
    {
        _activity = activity;
    }
    public  void print(String info)
    {

    }
    public  void setCurrentModuleKey(long k)
    {
        _curModuleKey = k;
    }
    public  long getCurrentModuleKey()
    {
        return _curModuleKey;
    }
    public  Object createLinearLayout()
    {
        LinearLayout layout = new LinearLayout(_activity);
        layout.setOrientation(LinearLayout.VERTICAL);
        return (Object)layout;
    }
    public  Object createTextview(String txt)
    {
        TextView tv =  new TextView(_activity);
        tv.setText(txt);
        return (Object) tv;
    }
    public  Object createButton(String txt)
    {
        Button btn =  new Button(_activity);
        btn.setTextColor(Color.RED);
        btn.setText(txt);
        return (Object) btn;
    }
    public  void setText(Object obj,String txt)
    {
        ((TextView)obj).setText(txt);
    }
    public  void setOnClickListener(Object obj,long handler)
    {
        ((View)obj).setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                Object[] params = new Object[1];
                params[0] = v;
                callJNI(_curModuleKey,handler,params);
            }
        });
    }
    public  View GetCurrentPage()
    {
        return _curPage;
    }
    public void addView(Object container,Object view)
    {
        ((ViewGroup)container).addView((View)view);
    }
    public Object createPage(String title)
    {
        ConstraintLayout layout = new ConstraintLayout(_activity);
        layout.setId(100);
        ConstraintLayout.LayoutParams params =
                new ConstraintLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT);
        layout.setLayoutParams(params);
        _curPage = layout;
        return layout;
    }
    public native boolean loadJNI();
    public native long loadModuleJNI(String code);
    public native boolean runJNI(String code);
    public  native boolean callJNI(long moduleKey,long callable,Object[] params );
    public native boolean unloadJNI();
}
