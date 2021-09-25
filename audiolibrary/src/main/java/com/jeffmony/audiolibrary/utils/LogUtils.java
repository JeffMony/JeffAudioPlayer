package com.jeffmony.audiolibrary.utils;

import android.util.Log;

/**
 * @author jeffli
 * @Date   2021-09-06
 */

public class LogUtils {

    private static final String PRE_LOG_TAG = "Audio_lib_";

    private static final boolean VERBOSE = false;
    private static final boolean DEBUG = false;
    private static final boolean INFO = true;
    private static final boolean WARN = true;
    private static final boolean ERROR = true;

    public static void v(String tag, String msg) {
        if (VERBOSE) {
            Log.v(PRE_LOG_TAG + tag, msg);
        }
    }

    public static void d(String tag, String msg) {
        if (DEBUG) {
            Log.d(PRE_LOG_TAG + tag, msg);
        }
    }

    public static void i(String tag, String msg) {
        if (INFO) {
            Log.i(PRE_LOG_TAG + tag, msg);
        }
    }

    public static void w(String tag, String msg) {
        if (WARN) {
            Log.w(PRE_LOG_TAG + tag, msg);
        }
    }

    public static void e(String tag, String msg) {
        if (ERROR) {
            Log.d(PRE_LOG_TAG + tag, msg);
        }
    }
}
