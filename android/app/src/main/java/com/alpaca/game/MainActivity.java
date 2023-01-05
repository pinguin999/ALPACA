package com.alpaca.game;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.NativeActivity;
import android.content.DialogInterface;
import android.content.pm.ApplicationInfo;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;

import java.lang.ref.WeakReference;
import java.util.concurrent.Semaphore;
import java.util.concurrent.atomic.AtomicInteger;

public class MainActivity extends NativeActivity {
    private HideHandler hideHandler;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // create a handler to set immersive mode on a delay
        hideHandler = new HideHandler(this);

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
        private final WeakReference<MainActivity> activity;

        HideHandler(MainActivity activity) {
            this.activity = new WeakReference<>(activity);
        }

        @Override
        public void handleMessage(Message msg) {
            MainActivity activity = this.activity.get();
            if (activity != null) {
                activity.setToImmersiveMode();
            }
        }
    }

    // Source: https://stackoverflow.com/a/60611870/647898
    /**
     * This function will be called from C++ by name and signature (Ljava/lang/String;Z)I)
     *
     * @param message the message text to show
     * @param model   if true, it will block the current thread, otherwise, it acts like a modeless dialog.
     * @return return id of the button that was clicked for a model dialog, otherwise, 0.
     * @see #showAlertCallback
     * @see <a href="https://stackoverflow.com/questions/11730001/create-a-message-dialog-in-android-via-ndk-callback/60611870#60611870">
     * Create a message dialog in Android via NDK callback</a>
     * @see <a href="https://stackoverflow.com/questions/6120567/android-how-to-get-a-modal-dialog-or-similar-modal-behavior">
     * Android: How to get a modal dialog or similar modal behavior?</a>
     */
    public int showAlert(final String message, boolean model) {
        //https://stackoverflow.com/questions/11411022/how-to-check-if-current-thread-is-not-main-thread
        if (Looper.myLooper() == Looper.getMainLooper() && model) {
            // Current Thread is UI Thread. Looper.getMainLooper().isCurrentThread()
            //android.os.NetworkOnMainThreadException
            throw new RuntimeException("Can't create a model dialog inside Main thread");
        }
        ApplicationInfo applicationInfo = getApplicationInfo();
        final CharSequence appName = getPackageManager().getApplicationLabel(applicationInfo);
        // Use a semaphore to create a modal dialog. Also, it's holden by the dialog's listener.
        final Semaphore semaphore = model ? new Semaphore(0, true) : null;
        // The button that was clicked (ex. BUTTON_POSITIVE) or the position of the item clicked
        final AtomicInteger buttonId = new AtomicInteger();
        this.runOnUiThread(new Runnable() {
            public void run() {
                AlertDialog.Builder builder = new AlertDialog.Builder(MainActivity.this, android.R.style.Theme_Material_Light_Dialog_Alert);
                builder.setTitle(appName);
                builder.setMessage(message);
                DialogInterface.OnClickListener listener = new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        buttonId.set(id);
                        if (null != semaphore)
                            semaphore.release();
                        if (DialogInterface.BUTTON_POSITIVE == id) {
                            MainActivity.this.finish();
                        }
                    }
                };
                builder.setPositiveButton(android.R.string.ok, listener);
                builder.setCancelable(false);
                AlertDialog dialog = builder.create();
                dialog.show();
            }
        });
        if (null != semaphore)
            try {
                semaphore.acquire();
            } catch (InterruptedException e) {
                Log.v("GameActivity", "ignored", e);
            }
        return buttonId.get();
    }

    /**
     * @see <a href="https://stackoverflow.com/questions/13822842/dialogfragment-with-clear-background-not-dimmed">
     * DialogFragment with clear background (not dimmed)</a>
     */
    protected void showDialog() {
        Dialog dialog = new Dialog(this);
        dialog.getWindow().requestFeature(Window.FEATURE_NO_TITLE);
        dialog.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        // layout to display
        //dialog.setContentView(R.layout.dialog_layout);

        // set color transpartent
        dialog.getWindow().setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));

        dialog.show();
    }
}
