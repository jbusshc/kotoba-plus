package com.kotobaplus.app;

import android.os.Bundle;
import androidx.core.splashscreen.SplashScreen;

import org.qtproject.qt.android.bindings.QtActivity;

public class MainActivity extends QtActivity {

    private boolean keepSplash = true;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        SplashScreen splashScreen = SplashScreen.installSplashScreen(this);

        super.onCreate(savedInstanceState);

        splashScreen.setKeepOnScreenCondition(() -> keepSplash);
    }

    // ── Llamado desde C++ cuando QML esté listo ──
    public void releaseSplash() {
        keepSplash = false;
    }
}