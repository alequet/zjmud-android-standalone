package com.zjmud.android

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Intent
import android.content.pm.ServiceInfo
import android.os.Build
import android.os.IBinder
import androidx.core.app.NotificationCompat
import androidx.core.app.ServiceCompat
import kotlin.concurrent.thread

class MudForegroundService : Service() {
    @Volatile
    private var driverThread: Thread? = null

    override fun onCreate() {
        super.onCreate()
        createNotificationChannel()
        ServiceCompat.startForeground(
            this,
            NOTIFICATION_ID,
            createNotification("本地服务正在启动"),
            if (Build.VERSION.SDK_INT >= 34) ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE else 0,
        )
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        val configPath = intent?.getStringExtra(EXTRA_CONFIG_PATH)
        if (configPath.isNullOrBlank()) {
            stopSelf(startId)
            return START_NOT_STICKY
        }
        if (driverThread?.isAlive != true) {
            driverThread = thread(name = "zjmud-driver") {
                val exitCode = runCatching { NativeBridge.runDriver(configPath) }.getOrDefault(-1)
                getSystemService(NotificationManager::class.java).notify(
                    NOTIFICATION_ID,
                    createNotification("本地服务已停止（$exitCode）"),
                )
            }
        }
        return START_NOT_STICKY
    }

    override fun onTaskRemoved(rootIntent: Intent?) {
        requestDriverStop()
        stopSelf()
        super.onTaskRemoved(rootIntent)
    }

    override fun onDestroy() {
        requestDriverStop()
        runCatching { driverThread?.join(DRIVER_STOP_TIMEOUT_MS) }
        ServiceCompat.stopForeground(this, ServiceCompat.STOP_FOREGROUND_REMOVE)
        super.onDestroy()
    }

    override fun onBind(intent: Intent?): IBinder? = null

    private fun requestDriverStop() {
        runCatching { NativeBridge.requestStop() }
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= 26) {
            getSystemService(NotificationManager::class.java).createNotificationChannel(
                NotificationChannel(
                    CHANNEL_ID,
                    "本地游戏服务",
                    NotificationManager.IMPORTANCE_LOW,
                ).apply {
                    description = "保持单机 MUD 服务在后台运行"
                    setShowBadge(false)
                },
            )
        }
    }

    private fun createNotification(status: String): Notification {
        val openIntent = PendingIntent.getActivity(
            this,
            0,
            Intent(this, MainActivity::class.java),
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE,
        )
        return NotificationCompat.Builder(this, CHANNEL_ID)
            .setSmallIcon(android.R.drawable.stat_sys_download_done)
            .setContentTitle("指间 MUD 单机版")
            .setContentText(status)
            .setContentIntent(openIntent)
            .setOngoing(true)
            .setOnlyAlertOnce(true)
            .build()
    }

    companion object {
        const val EXTRA_CONFIG_PATH = "config_path"
        private const val CHANNEL_ID = "mud_server"
        private const val NOTIFICATION_ID = 3000
        private const val DRIVER_STOP_TIMEOUT_MS = 5_000L
    }
}
