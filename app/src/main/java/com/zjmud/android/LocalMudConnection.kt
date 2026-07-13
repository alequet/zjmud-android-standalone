package com.zjmud.android

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.cancelAndJoin
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import java.io.BufferedReader
import java.io.InputStreamReader
import java.net.InetSocketAddress
import java.net.Socket
import java.nio.charset.Charset
import java.nio.charset.CodingErrorAction
import java.util.concurrent.LinkedBlockingQueue
import java.util.concurrent.TimeUnit

class LocalMudConnection(
    private val ownerScope: CoroutineScope,
    private val onEvent: (event: String, payload: String) -> Unit,
) {
    private val charset = Charset.forName("GB18030")
    private val writes = LinkedBlockingQueue<String>()
    private var socket: Socket? = null
    private var job: Job? = null

    fun start() {
        if (job != null) return
        job = ownerScope.launch(Dispatchers.IO) {
            onEvent("status", "正在启动本地服务...\n")
            var failures = 0
            while (isActive) {
                val result = runCatching { connectAndRun() }
                if (!isActive) break

                failures = if (result.isFailure) failures + 1 else 0
                if (failures == CONNECT_FAILURE_NOTICE) {
                    onEvent("status", "本地服务暂时不可用，正在继续重连...\n")
                }
                delay(
                    (CONNECT_DELAY_MS + failures * 50L)
                        .coerceAtMost(MAX_CONNECT_DELAY_MS),
                )
            }
        }
    }

    fun send(text: String) {
        if (text.length <= MAX_COMMAND_LENGTH) writes.offer(text)
    }

    fun close() {
        job?.cancel()
        job = null
        runCatching { socket?.close() }
        socket = null
        writes.clear()
    }

    private suspend fun connectAndRun() {
        val activeSocket = Socket().apply {
            tcpNoDelay = true
            connect(InetSocketAddress(LOOPBACK_HOST, MUD_PORT), SOCKET_TIMEOUT_MS)
        }
        writes.clear()
        socket = activeSocket
        val output = activeSocket.getOutputStream().buffered()
        output.write(WEB_HANDSHAKE.toByteArray(charset))
        output.flush()
        onEvent("connected", "")
        onEvent("status", "本地服务连接成功。\n")

        val writer = ownerScope.launch(Dispatchers.IO) {
            while (isActive && !activeSocket.isClosed) {
                val command = writes.poll(WRITER_POLL_MS, TimeUnit.MILLISECONDS) ?: continue
                val written = runCatching {
                    output.write(command.toByteArray(charset))
                    output.flush()
                }.isSuccess
                if (!written) break
            }
        }

        try {
            val decoder = charset.newDecoder()
                .onMalformedInput(CodingErrorAction.REPLACE)
                .onUnmappableCharacter(CodingErrorAction.REPLACE)
            BufferedReader(InputStreamReader(activeSocket.getInputStream(), decoder)).use { reader ->
                val chars = CharArray(READ_BUFFER_CHARS)
                while (ownerScope.isActive) {
                    val count = reader.read(chars)
                    if (count < 0) break
                    if (count > 0) onEvent("stream", String(chars, 0, count))
                }
            }
        } finally {
            writer.cancelAndJoin()
            runCatching { activeSocket.close() }
            socket = null
            writes.clear()
            onEvent("disconnect", "")
        }
    }

    private companion object {
        const val LOOPBACK_HOST = "127.0.0.1"
        const val MUD_PORT = 3000
        const val SOCKET_TIMEOUT_MS = 250
        const val CONNECT_FAILURE_NOTICE = 60
        const val CONNECT_DELAY_MS = 100L
        const val MAX_CONNECT_DELAY_MS = 1_000L
        const val READ_BUFFER_CHARS = 4096
        const val WRITER_POLL_MS = 100L
        const val MAX_COMMAND_LENGTH = 64 * 1024
        const val WEB_HANDSHAKE = "zjmDMaIpOvxdb\n\n"
    }
}
