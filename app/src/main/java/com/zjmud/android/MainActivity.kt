package com.zjmud.android

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.view.Gravity
import android.view.ViewGroup
import android.webkit.WebResourceRequest
import android.webkit.WebResourceResponse
import android.webkit.WebView
import android.webkit.WebViewClient
import android.widget.TextView
import androidx.core.content.ContextCompat
import androidx.webkit.JavaScriptReplyProxy
import androidx.webkit.WebViewAssetLoader
import androidx.webkit.WebViewCompat
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch
import org.json.JSONObject

class MainActivity : Activity() {
    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Main.immediate)
    private var connection: LocalMudConnection? = null
    private var webView: WebView? = null
    private var replyProxy: JavaScriptReplyProxy? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        showStatus("正在准备本地游戏资源...")
        requestNotificationPermission()

        scope.launch {
            runCatching { RuntimeDeployer.deploy(this@MainActivity) }
                .onSuccess { runtime ->
                    ContextCompat.startForegroundService(
                        this@MainActivity,
                        Intent(this@MainActivity, MudForegroundService::class.java)
                            .putExtra(MudForegroundService.EXTRA_CONFIG_PATH, runtime.configFile.absolutePath),
                    )
                    showGame()
                }
                .onFailure { error ->
                    showStatus("游戏资源准备失败\n\n${error.message}")
                }
        }
    }

    @Suppress("SetJavaScriptEnabled")
    private fun showGame() {
        val assetLoader = WebViewAssetLoader.Builder()
            .addPathHandler("/assets/", WebViewAssetLoader.AssetsPathHandler(this))
            .build()

        val gameView = WebView(this).apply {
            layoutParams = ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT,
            )
            setBackgroundColor(0xff000000.toInt())
            settings.javaScriptEnabled = true
            settings.domStorageEnabled = true
            settings.allowFileAccess = false
            settings.allowContentAccess = false
            settings.javaScriptCanOpenWindowsAutomatically = false
            settings.setSupportMultipleWindows(false)
            webViewClient = object : WebViewClient() {
                override fun shouldInterceptRequest(view: WebView, request: WebResourceRequest): WebResourceResponse? {
                    return localResponse(assetLoader, request.url)
                }

                @Deprecated("Legacy WebView callback")
                override fun shouldInterceptRequest(view: WebView, url: String): WebResourceResponse? {
                    return localResponse(assetLoader, Uri.parse(url))
                }

                override fun shouldOverrideUrlLoading(view: WebView, request: WebResourceRequest): Boolean {
                    return !isLocalAsset(request.url)
                }
            }
        }

        WebViewCompat.addWebMessageListener(
            gameView,
            "zjmudNative",
            setOf(APP_ASSET_ORIGIN),
        ) { _, message, _, isMainFrame, proxy ->
            if (!isMainFrame) return@addWebMessageListener
            replyProxy = proxy
            handleWebMessage(message.data.orEmpty())
        }

        webView = gameView
        setContentView(gameView)
        gameView.loadUrl(GAME_URL)
    }

    private fun localResponse(loader: WebViewAssetLoader, uri: Uri): WebResourceResponse? {
        if (!isLocalAsset(uri)) {
            return WebResourceResponse("text/plain", "UTF-8", 403, "Offline only", emptyMap(), null)
        }
        return loader.shouldInterceptRequest(uri)
    }

    private fun isLocalAsset(uri: Uri): Boolean =
        uri.scheme == "https" && uri.host == APP_ASSET_HOST

    private fun handleWebMessage(rawMessage: String) {
        val message = runCatching { JSONObject(rawMessage) }.getOrNull() ?: return
        when (message.optString("type")) {
            "ready" -> startConnection()
            "emit" -> if (message.optString("event") == "stream") {
                connection?.send(message.optString("payload"))
            }
        }
    }

    private fun startConnection() {
        connection?.close()
        connection = LocalMudConnection(scope) { event, payload ->
            val message = JSONObject()
                .put("event", event)
                .put("payload", payload)
                .toString()
            runOnUiThread { replyProxy?.postMessage(message) }
        }.also { it.start() }
    }

    private fun showStatus(message: String) {
        setContentView(TextView(this).apply {
            gravity = Gravity.CENTER
            setPadding(48, 48, 48, 48)
            text = message
            textSize = 17f
        })
    }

    private fun requestNotificationPermission() {
        if (Build.VERSION.SDK_INT >= 33 &&
            checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED
        ) {
            requestPermissions(arrayOf(Manifest.permission.POST_NOTIFICATIONS), NOTIFICATION_PERMISSION_REQUEST)
        }
    }

    override fun onDestroy() {
        if (isFinishing && !isChangingConfigurations) {
            stopService(Intent(this, MudForegroundService::class.java))
        }
        connection?.close()
        webView?.destroy()
        scope.cancel()
        super.onDestroy()
    }

    private companion object {
        const val APP_ASSET_HOST = "appassets.androidplatform.net"
        const val APP_ASSET_ORIGIN = "https://$APP_ASSET_HOST"
        const val GAME_URL = "$APP_ASSET_ORIGIN/assets/web/index.html"
        const val NOTIFICATION_PERMISSION_REQUEST = 100
    }
}
