package org.xlangfoundation.playground;

import android.app.Fragment;
import android.graphics.Color;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
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
        Log.v("xlangPrint", info);
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
        layout.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));
        return (Object)layout;
    }
    public  Object createTextview(String txt)
    {
        TextView tv =  new TextView(_activity);
        tv.setText(txt);
        return (Object) tv;
    }
    public  Object createEditText(String txt)
    {
        EditText tv =  new EditText(_activity);
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
        if(obj != null)
        {
            ((TextView)obj).setText(txt);
        }
    }
    public Object getLayoutParams(Object obj)
    {
        //ViewGroup.LayoutParams
        //LinearLayout.LayoutParams
        //ConstraintLayout.LayoutParams
        return ((View)obj).getLayoutParams();
    }
    public  void setTextColor(Object obj,int color)
    {
        ((TextView)obj).setTextColor(color);
    }
    public  void setTextSize(Object obj,int unit, float size)
    {
        ((TextView)obj).setTextSize(unit,size);
    }
    public  void setBackgroundColor(Object obj,int color)
    {
        ((View)obj).setBackgroundColor(color);
    }
    public  String getText(Object obj)
    {
        return ((TextView)obj).getText().toString();
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
    public  void setOnTextChangedListener(Object obj,long handler)
    {
        ((TextView)obj).addTextChangedListener(new TextWatcher() {

            @Override
            public void afterTextChanged(Editable s) {

            }

            @Override
            public void beforeTextChanged(CharSequence s, int start,
                                          int count, int after) {
            }

            @Override
            public void onTextChanged(CharSequence s, int start,
                                      int before, int count) {
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
                        ViewGroup.LayoutParams.MATCH_PARENT);
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
