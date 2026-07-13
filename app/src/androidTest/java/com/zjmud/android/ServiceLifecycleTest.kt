package com.zjmud.android

import android.app.ActivityManager
import android.content.Intent
import android.view.View
import android.view.ViewGroup
import android.webkit.WebView
import androidx.core.content.ContextCompat
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.Assert.assertTrue
import org.junit.Assert.assertFalse
import org.junit.Test
import org.junit.runner.RunWith
import java.io.File
import java.net.InetSocketAddress
import java.net.Socket
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import org.json.JSONTokener

@RunWith(AndroidJUnit4::class)
class ServiceLifecycleTest {
    @Test
    fun testStopServiceRunsGracefulShutdown() {
        val context = InstrumentationRegistry.getInstrumentation().targetContext
        val configPath = context.filesDir.resolve("runtime/current/config.android.ini").absolutePath
        ContextCompat.startForegroundService(
            context,
            Intent(context, MudForegroundService::class.java)
                .putExtra(MudForegroundService.EXTRA_CONFIG_PATH, configPath),
        )
        Thread.sleep(5_000)
        val stopped = context.stopService(
            Intent(context, MudForegroundService::class.java),
        )
        assertTrue("MUD foreground service was not running", stopped)
    }

    @Test
    fun testRemovingTaskSavesOnlineUserAndStopsServer() {
        val instrumentation = InstrumentationRegistry.getInstrumentation()
        val context = instrumentation.targetContext
        val driverLog = context.filesDir.resolve("runtime/current/android-driver.log")
        val initialLogSize = driverLog.length()
        val activityMonitor = instrumentation.addMonitor(MainActivity::class.java.name, null, false)

        context.startActivity(
            Intent(context, MainActivity::class.java)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK),
        )
        val activity = instrumentation.waitForMonitorWithTimeout(activityMonitor, 15_000)
        assertTrue("MainActivity did not start", activity != null)

        waitUntil(75_000, "online user was not autosaved") {
            driverLog.readTailFrom(initialLogSize).contains("Android autosave processed 1 interactive user(s).")
        }
        val newDriverLog = driverLog.readTailFrom(initialLogSize)
        listOf(
            "Unused local variable",
            "Unknown #pragma",
            "Number of arguments disagrees",
            "Expression has no side effects",
        ).forEach { diagnostic ->
            assertTrue("Expected compiler diagnostic was not exercised: $diagnostic", newDriverLog.contains(diagnostic))
        }
        val pageText = readWebViewText(activity!!)
        listOf(
            "编译时段错误",
            "Warning:",
            "Unused local variable",
            "Unknown #pragma",
            "Number of arguments disagrees",
            "Expression has no side effects",
        ).forEach { diagnostic ->
            assertFalse("Compiler diagnostic leaked into the game UI: $diagnostic", pageText.contains(diagnostic))
        }
        val saveFile = context.filesDir.resolve("runtime/current/data/user")
            .walkTopDown()
            .filter { it.isFile && it.extension == "o" }
            .maxByOrNull(File::lastModified)
        assertTrue("No character save file was found", saveFile != null)
        val savedAt = saveFile!!.lastModified()
        val shutdownLogSize = driverLog.length()
        Thread.sleep(1_500)

        val activityManager = context.getSystemService(ActivityManager::class.java)
        val gameTask = activityManager.appTasks.firstOrNull {
            it.taskInfo.baseIntent.component?.packageName == context.packageName
        }
        assertTrue("The game task was not running", gameTask != null)
        gameTask!!.finishAndRemoveTask()

        waitUntil(10_000, "local server port remained open after task removal") { !isMudPortOpen() }
        waitUntil(5_000, "graceful shutdown was not logged") {
            driverLog.readTailFrom(shutdownLogSize).contains("Android graceful shutdown started.")
        }
        assertTrue("Character was not saved during task removal", saveFile.lastModified() > savedAt)
    }

    private fun waitUntil(timeoutMs: Long, failureMessage: String, condition: () -> Boolean) {
        val deadline = System.currentTimeMillis() + timeoutMs
        while (System.currentTimeMillis() < deadline) {
            if (condition()) return
            Thread.sleep(200)
        }
        throw AssertionError(failureMessage)
    }

    private fun isMudPortOpen(): Boolean = runCatching {
        Socket().use { it.connect(InetSocketAddress("127.0.0.1", 3000), 100) }
    }.isSuccess

    private fun readWebViewText(activity: android.app.Activity): String {
        val webView = findWebView(activity.window.decorView)
        assertTrue("Game WebView was not found", webView != null)
        val latch = CountDownLatch(1)
        var pageText = ""
        activity.runOnUiThread {
            webView!!.evaluateJavascript("document.body.innerText") { json ->
                pageText = runCatching { JSONTokener(json).nextValue() as? String }.getOrNull().orEmpty()
                latch.countDown()
            }
        }
        assertTrue("Timed out while reading game UI", latch.await(5, TimeUnit.SECONDS))
        return pageText
    }

    private fun findWebView(view: View): WebView? {
        if (view is WebView) return view
        if (view !is ViewGroup) return null
        for (index in 0 until view.childCount) {
            findWebView(view.getChildAt(index))?.let { return it }
        }
        return null
    }

    private fun File.readTailFrom(offset: Long): String {
        if (!isFile || length() <= offset) return ""
        return inputStream().use { input ->
            var remaining = offset
            while (remaining > 0) {
                val skipped = input.skip(remaining)
                if (skipped <= 0) break
                remaining -= skipped
            }
            input.bufferedReader(Charsets.ISO_8859_1).readText()
        }
    }
}
