package com.zjmud.android

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import androidx.core.content.ContextCompat

class FaultInjectionReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        if (intent.action != ACTION_KILL_MUD_PROCESS) return
        ContextCompat.startForegroundService(
            context,
            Intent(context, MudForegroundService::class.java)
                .putExtra(MudForegroundService.EXTRA_FAULT_KILL_PROCESS, true),
        )
    }

    companion object {
        const val ACTION_KILL_MUD_PROCESS = "com.zjmud.android.DEBUG_KILL_MUD_PROCESS"
    }
}
