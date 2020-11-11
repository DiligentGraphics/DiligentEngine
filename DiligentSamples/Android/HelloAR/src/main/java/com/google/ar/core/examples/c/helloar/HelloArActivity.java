/*
 * Copyright 2017 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.ar.core.examples.c.helloar;

import android.hardware.display.DisplayManager;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Handler;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.Toast;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * This is a simple example that shows how to create an augmented reality (AR) application using the
 * ARCore C API.
 */
public class HelloArActivity extends AppCompatActivity
    implements GLSurfaceView.Renderer, DisplayManager.DisplayListener {
  private static final String TAG = HelloArActivity.class.getSimpleName();
  private static final int SNACKBAR_UPDATE_INTERVAL_MILLIS = 1000; // In milliseconds.

  private GLSurfaceView surfaceView;

  private boolean viewportChanged = false;
  private int viewportWidth;
  private int viewportHeight;

  // Opaque native pointer to the native application instance.
  private long nativeApplication;
  private GestureDetector gestureDetector;

  private Snackbar loadingMessageSnackbar;
  private Handler planeStatusCheckingHandler;
  private final Runnable planeStatusCheckingRunnable =
      new Runnable() {
        @Override
        public void run() {
          // The runnable is executed on main UI thread.
          try {
            if (JniInterface.hasDetectedPlanes(nativeApplication)) {
              if (loadingMessageSnackbar != null) {
                loadingMessageSnackbar.dismiss();
              }
              loadingMessageSnackbar = null;
            } else {
              planeStatusCheckingHandler.postDelayed(
                  planeStatusCheckingRunnable, SNACKBAR_UPDATE_INTERVAL_MILLIS);
            }
          } catch (Exception e) {
            Log.e(TAG, e.getMessage());
          }
        }
      };

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    surfaceView = (GLSurfaceView) findViewById(R.id.surfaceview);

    // Set up tap listener.
    gestureDetector =
        new GestureDetector(
            this,
            new GestureDetector.SimpleOnGestureListener() {
              @Override
              public boolean onSingleTapUp(final MotionEvent e) {
                surfaceView.queueEvent(
                    () -> JniInterface.onTouched(nativeApplication, e.getX(), e.getY()));
                return true;
              }

              @Override
              public boolean onDown(MotionEvent e) {
                return true;
              }
            });

    surfaceView.setOnTouchListener(
        (View v, MotionEvent event) -> gestureDetector.onTouchEvent(event));

    // Set up renderer.
    surfaceView.setPreserveEGLContextOnPause(true);
    surfaceView.setEGLContextClientVersion(3);
    surfaceView.setEGLConfigChooser(8, 8, 8, 8, 16, 0); // Alpha used for plane blending.
    surfaceView.setRenderer(this);
    surfaceView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
    surfaceView.setWillNotDraw(false);

    JniInterface.assetManager = getAssets();
    nativeApplication = JniInterface.createNativeApplication(getAssets());

    planeStatusCheckingHandler = new Handler();
  }

  @Override
  protected void onResume() {
    super.onResume();
    // ARCore requires camera permissions to operate. If we did not yet obtain runtime
    // permission on Android M and above, now is a good time to ask the user for it.
    if (!CameraPermissionHelper.hasCameraPermission(this)) {
      CameraPermissionHelper.requestCameraPermission(this);
      return;
    }

    JniInterface.onResume(nativeApplication, getApplicationContext(), this);
    surfaceView.onResume();

    loadingMessageSnackbar =
        Snackbar.make(
            HelloArActivity.this.findViewById(android.R.id.content),
            "Searching for surfaces...",
            Snackbar.LENGTH_INDEFINITE);
    // Set the snackbar background to light transparent black color.
    loadingMessageSnackbar.getView().setBackgroundColor(0xbf323232);
    loadingMessageSnackbar.show();
    planeStatusCheckingHandler.postDelayed(
        planeStatusCheckingRunnable, SNACKBAR_UPDATE_INTERVAL_MILLIS);

    // Listen to display changed events to detect 180° rotation, which does not cause a config
    // change or view resize.
    getSystemService(DisplayManager.class).registerDisplayListener(this, null);
  }

  @Override
  public void onPause() {
    super.onPause();
    surfaceView.onPause();
    JniInterface.onPause(nativeApplication);

    planeStatusCheckingHandler.removeCallbacks(planeStatusCheckingRunnable);

    getSystemService(DisplayManager.class).unregisterDisplayListener(this);
  }

  @Override
  public void onDestroy() {
    super.onDestroy();

    // Synchronized to avoid racing onDrawFrame.
    synchronized (this) {
      JniInterface.destroyNativeApplication(nativeApplication);
      nativeApplication = 0;
    }
  }

  @Override
  public void onWindowFocusChanged(boolean hasFocus) {
    super.onWindowFocusChanged(hasFocus);
    if (hasFocus) {
      // Standard Android full-screen functionality.
      getWindow()
          .getDecorView()
          .setSystemUiVisibility(
              View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                  | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                  | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                  | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                  | View.SYSTEM_UI_FLAG_FULLSCREEN
                  | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
      getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }
  }

  @Override
  public void onSurfaceCreated(GL10 gl, EGLConfig config) {
    GLES20.glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    JniInterface.onGlSurfaceCreated(nativeApplication);
  }

  @Override
  public void onSurfaceChanged(GL10 gl, int width, int height) {
    viewportWidth = width;
    viewportHeight = height;
    viewportChanged = true;
  }

  @Override
  public void onDrawFrame(GL10 gl) {
    // Synchronized to avoid racing onDestroy.
    synchronized (this) {
      if (nativeApplication == 0) {
        return;
      }
      if (viewportChanged) {
        int displayRotation = getWindowManager().getDefaultDisplay().getRotation();
        JniInterface.onDisplayGeometryChanged(
            nativeApplication, displayRotation, viewportWidth, viewportHeight);
        viewportChanged = false;
      }
      JniInterface.onGlSurfaceDrawFrame(nativeApplication);
    }
  }

  @Override
  public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] results) {
    if (!CameraPermissionHelper.hasCameraPermission(this)) {
      Toast.makeText(this, "Camera permission is needed to run this application", Toast.LENGTH_LONG)
          .show();
      if (!CameraPermissionHelper.shouldShowRequestPermissionRationale(this)) {
        // Permission denied with checking "Do not ask again".
        CameraPermissionHelper.launchPermissionSettings(this);
      }
      finish();
    }
  }

  // DisplayListener methods
  @Override
  public void onDisplayAdded(int displayId) {}

  @Override
  public void onDisplayRemoved(int displayId) {}

  @Override
  public void onDisplayChanged(int displayId) {
    viewportChanged = true;
  }
}
