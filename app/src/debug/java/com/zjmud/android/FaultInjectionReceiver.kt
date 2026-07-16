package com.zjmud.android

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.os.Process
import androidx.core.content.ContextCompat

class FaultInjectionReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        when (intent.action) {
            ACTION_KILL_MUD_PROCESS -> ContextCompat.startForegroundService(
                context,
                Intent(context, MudForegroundService::class.java)
                    .putExtra(MudForegroundService.EXTRA_FAULT_KILL_PROCESS, true),
            )
            ACTION_KILL_UI_PROCESS -> Process.killProcess(Process.myPid())
        }
    }

    companion object {
        const val ACTION_KILL_MUD_PROCESS = "com.zjmud.android.DEBUG_KILL_MUD_PROCESS"
        const val ACTION_KILL_UI_PROCESS = "com.zjmud.android.DEBUG_KILL_UI_PROCESS"
    }
}
