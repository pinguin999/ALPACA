package com.alpaca.game;

import android.app.NativeActivity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.View;

import java.lang.ref.WeakReference;

public class TvActivity extends NativeActivity {
    private TvActivity.HideHandler hideHandler;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // create a handler to set immersive mode on a delay
        hideHandler = new TvActivity.HideHandler(this);

        getWindow().getDecorView().setOnSystemUiVisibilityChangeListener
                (new View.OnSystemUiVisibilityChangeListener() {
                    @Override
                    public void onSystemUiVisibilityChange(int visibility) {
                        if (visibility == View.SYSTEM_UI_FLAG_VISIBLE) {
                            hideHandler.removeMessages(0);
                            hideHandler.sendEmptyMessageDelayed(0, 300);
                        }
                    }
                });
    }

    @Override
    protected void onResume() {
        super.onResume();
        setToImmersiveMode();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            hideHandler.removeMessages(0);
            hideHandler.sendEmptyMessageDelayed(0, 300);
        } else {
            hideHandler.removeMessages(0);
        }
    }

    private void setToImmersiveMode() {
        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
    }

    private static class HideHandler extends Handler {
        private final WeakReference<TvActivity> activity;

        HideHandler(TvActivity activity) {
            this.activity = new WeakReference<>(activity);
        }

        @Override
        public void handleMessage(Message msg) {
            TvActivity activity = this.activity.get();
            if (activity != null) {
                activity.setToImmersiveMode();
            }
        }
    }

}
