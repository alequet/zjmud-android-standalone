package com.zjmud.android

import android.content.Context
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.File
import java.util.Properties
import java.util.zip.ZipInputStream

data class MudRuntime(
    val root: File,
    val configFile: File,
)

object RuntimeDeployer {
    private const val RUNTIME_ASSET = "runtime/zjmud-runtime.zip"
    private const val MANIFEST_ASSET = "runtime/manifest.properties"
    private const val MARKER_FILE = ".payload-id"
    private const val WIZLIST_PATH = "adm/etc/wizlist"
    private val MUTABLE_DIRECTORIES = listOf("adm/tmp", "backup", "data", "dump", "log", "temp")

    suspend fun deploy(context: Context): MudRuntime = withContext(Dispatchers.IO) {
        val properties = Properties().apply {
            context.assets.open(MANIFEST_ASSET).use(::load)
        }
        val payloadId = requireNotNull(properties.getProperty("runtime_sha256"))
        val runtimeParent = File(context.filesDir, "runtime").apply { mkdirs() }
        val runtimeRoot = File(runtimeParent, "current")
        val previousRoot = File(runtimeParent, "previous")
        val marker = File(runtimeRoot, MARKER_FILE)

        if (!runtimeRoot.exists() && previousRoot.exists()) {
            check(previousRoot.renameTo(runtimeRoot)) { "无法恢复上一次游戏资源。" }
        }
        if (runtimeRoot.exists() && previousRoot.exists()) {
            previousRoot.deleteRecursively()
            check(!previousRoot.exists()) { "无法清理旧版游戏资源。" }
        }

        if (!runtimeRoot.exists()) {
            deployFresh(context, runtimeParent, runtimeRoot, payloadId)
        } else if (marker.readTextOrNull() != payloadId) {
            migrateRuntime(context, runtimeParent, runtimeRoot, previousRoot, payloadId)
        }

        applyCompatibilityPatches(runtimeRoot)
        val configFile = createAndroidConfig(runtimeRoot)
        MudRuntime(runtimeRoot, configFile)
    }

    private fun deployFresh(context: Context, parent: File, root: File, payloadId: String) {
        val staging = File(parent, "deploy-$payloadId").apply { deleteRecursively() }
        staging.mkdirs()
        try {
            extractAsset(context, staging)
            createWritableDirectories(staging)
            File(staging, MARKER_FILE).writeText(payloadId)
            check(staging.renameTo(root)) { "无法完成游戏资源部署。" }
        } catch (error: Throwable) {
            staging.deleteRecursively()
            throw error
        }
    }

    private fun migrateRuntime(
        context: Context,
        parent: File,
        current: File,
        previous: File,
        payloadId: String,
    ) {
        val staging = File(parent, "deploy-$payloadId").apply { deleteRecursively() }
        check(!previous.exists()) { "发现未完成的游戏资源升级。" }
        staging.mkdirs()
        try {
            extractAsset(context, staging)
            createWritableDirectories(staging)
            MUTABLE_DIRECTORIES.forEach { relativePath ->
                val source = File(current, relativePath)
                if (source.exists()) {
                    check(source.copyRecursively(File(staging, relativePath), overwrite = true)) {
                        "无法迁移存档目录：$relativePath"
                    }
                }
            }
            mergeWizardList(File(current, WIZLIST_PATH), File(staging, WIZLIST_PATH))
            File(staging, MARKER_FILE).writeText(payloadId)
            check(current.renameTo(previous)) { "无法备份当前游戏资源。" }
            if (!staging.renameTo(current)) {
                check(previous.renameTo(current)) { "游戏资源升级失败，且无法恢复旧版本。" }
                error("无法启用新版游戏资源，已恢复旧版本。")
            }
            previous.deleteRecursively()
        } catch (error: Throwable) {
            staging.deleteRecursively()
            if (!current.exists() && previous.exists()) previous.renameTo(current)
            throw error
        }
    }

    private fun extractAsset(context: Context, destination: File) {
        context.assets.open(RUNTIME_ASSET).use { input ->
            ZipInputStream(input.buffered()).use { zip -> extract(zip, destination) }
        }
    }

    private fun mergeWizardList(current: File, staging: File) {
        val statuses = linkedMapOf<String, String>()
        listOf(staging, current).forEach { file ->
            if (!file.isFile) return@forEach
            file.readLines(Charsets.ISO_8859_1).forEach { line ->
                val parts = line.trim().split(Regex("\\s+"), limit = 2)
                if (parts.size == 2 && !parts[0].startsWith("#")) {
                    statuses[parts[0]] = parts[1]
                }
            }
        }
        staging.parentFile?.mkdirs()
        staging.writeText(
            statuses.entries.joinToString(separator = "", transform = { "${it.key} ${it.value}\n" }),
            Charsets.ISO_8859_1,
        )
    }

    private fun applyCompatibilityPatches(root: File) {
        ensureIncludeGuard(File(root, "include/ansi.h"), "ZJMUD_ANSI_H")
        ensureIncludeGuard(File(root, "include/zjmud.h"), "ZJMUD_PROTOCOL_H")
        ensureIncludeGuard(File(root, "include/globals.h"), "ZJMUD_GLOBALS_H")
    }

    private fun ensureIncludeGuard(file: File, guard: String) {
        check(file.isFile) { "兼容补丁目标缺失：${file.path}" }
        val content = file.readBytes().toString(Charsets.ISO_8859_1)
        val prefix = "#ifndef $guard\n#define $guard\n"
        if (content.startsWith(prefix)) return
        file.writeBytes((prefix + content + "\n#endif\n").toByteArray(Charsets.ISO_8859_1))
    }

    private fun extract(zip: ZipInputStream, destination: File) {
        val rootPath = destination.canonicalFile.toPath()
        while (true) {
            val entry = zip.nextEntry ?: break
            val relativePath = entry.name.removePrefix("./")
            if (relativePath.isEmpty()) continue
            val output = File(destination, relativePath).canonicalFile
            check(output.toPath().startsWith(rootPath)) { "资源包包含非法路径：${entry.name}" }
            if (entry.isDirectory) {
                output.mkdirs()
            } else {
                output.parentFile?.mkdirs()
                output.outputStream().buffered().use { zip.copyTo(it) }
            }
            zip.closeEntry()
        }
    }

    private fun createWritableDirectories(root: File) {
        (MUTABLE_DIRECTORIES + "binaries")
            .forEach { File(root, it).mkdirs() }
    }

    private fun createAndroidConfig(root: File): File {
        val original = File(root, "config.ini")
        check(original.isFile) { "游戏配置文件缺失。" }
        var config = original.readBytes().toString(Charsets.ISO_8859_1)
        config = config.replace(
            Regex("(?m)^mudlib directory\\s*:.*$"),
            "mudlib directory : ${root.absolutePath}",
        ).replace(
            Regex("(?m)^binary directory\\s*:.*$"),
            "binary directory : ${root.absolutePath}",
        )
        if (!Regex("(?m)^mud ip\\s*:").containsMatchIn(config)) {
            config = config.replace(
                Regex("(?m)^(port number\\s*:.*)$"),
                "$1\nmud ip : 127.0.0.1",
            )
        }
        config = config.replace(
            Regex("(?m)^global include file\\s*:.*(?:\\r?\\n)?"),
            "",
        ).trimEnd() + "\nglobal include file : \"/include/globals.h\"\n"
        return File(root, "config.android.ini").apply {
            writeBytes(config.toByteArray(Charsets.ISO_8859_1))
        }
    }

    private fun File.readTextOrNull(): String? =
        if (isFile) runCatching { readText() }.getOrNull() else null
}
