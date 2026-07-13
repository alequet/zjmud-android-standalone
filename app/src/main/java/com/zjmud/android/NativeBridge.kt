package com.zjmud.android

object NativeBridge {
    init {
        System.loadLibrary("zjmud_native")
    }

    external fun version(): String
    external fun runDriver(configPath: String): Int
    external fun requestStop()
}

