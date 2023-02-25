package org.xlangfoundation.playground;

import android.app.Fragment;
import android.graphics.Color;
import android.os.Handler;
import android.os.Looper;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
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
    private ViewGroup _ContentHolder = null;
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
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT);
        lp.gravity= Gravity.CENTER;
        //lp.setMargins(100, 20, 100, 10);
        //layout.setPadding(100,20,100,10);
        layout.setLayoutParams(lp);
        return (Object)layout;
    }
    public void setMargins (Object obj, int l, int t, int r, int b)
    {
        View v = (View)obj;
        if (v.getLayoutParams() instanceof ViewGroup.MarginLayoutParams)
        {
            ViewGroup.MarginLayoutParams p = (ViewGroup.MarginLayoutParams)v.getLayoutParams();
            p.setMargins(l, t, r, b);
            v.setLayoutParams(p);
        }
        else
        {
            ViewGroup.MarginLayoutParams p = new ViewGroup.MarginLayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT);
            p.setMargins(l, t, r, b);
            v.setLayoutParams(p);
        }
    }
    public  Object createScrollview()
    {
        ScrollView v =  new ScrollView(_activity);
        v.setLayoutParams(
                new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.MATCH_PARENT));
        return (Object) v;
    }
    public  Object createTextview(String txt)
    {
        TextView tv =  new TextView(_activity);
        tv.setWidth(200);
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
        btn.setWidth(100);
        //setMargins(btn,100,100,100,100);
        btn.setGravity(Gravity.CENTER);
        return (Object) btn;
    }
    public  void PostCallFromUIThread()
    {
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            @Override
            public void run() {
                callFromUIThreadJNI();
            }
        });
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
    public void SetContentHolder(ViewGroup v)
    {
        _ContentHolder = v;
    }
    public  View GetCurrentPage()
    {
        return _ContentHolder;
    }
    public void addView(Object container,Object view)
    {
        ((ViewGroup)container).addView((View)view);
    }
    public Object createPage(String title)
    {
        ConstraintLayout layout = new ConstraintLayout(_activity);
        //layout.setId(100);
        ConstraintLayout.LayoutParams params =
                new ConstraintLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.MATCH_PARENT);
        layout.setLayoutParams(params);
        return layout;
    }
    public void showPage(Object page)
    {
        if(_ContentHolder != null)
        {
            _ContentHolder.removeAllViews();
            _ContentHolder.addView((View)page);
        }
    }
    public native boolean loadJNI();
    public native long loadModuleJNI(String code);
    public native boolean runJNI(String code);
    public  native boolean callJNI(long moduleKey,long callable,Object[] params );
    public  native boolean callFromUIThreadJNI();
    public native boolean unloadJNI();
}
