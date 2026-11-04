// SPDX-License-Identifier: GPL-3.0-only
// JIANT Stems: hands an export to the share sheet (Drive, WhatsApp, mail, Files): a read-only content:// address
// for a file in the app's cache/exports, readable by the app picked through the temporary grant the share intent
// carries. (What AndroidX's FileProvider does, without AndroidX.)
package fm.jiant.stems

import android.content.ContentProvider
import android.content.ContentValues
import android.content.Context
import android.database.Cursor
import android.database.MatrixCursor
import android.net.Uri
import android.os.ParcelFileDescriptor
import android.provider.OpenableColumns
import java.io.File
import java.io.FileNotFoundException

class ExportProvider : ContentProvider() {
    override fun onCreate() = true

    private fun file(uri: Uri): File {
        val ctx = context ?: throw FileNotFoundException()
        val name = uri.lastPathSegment ?: throw FileNotFoundException()
        val f = File(dir(ctx), name)
        if (f.parentFile?.canonicalPath != dir(ctx).canonicalPath || !f.isFile) throw FileNotFoundException(name)
        return f
    }

    override fun openFile(uri: Uri, mode: String): ParcelFileDescriptor {
        if (mode != "r") throw SecurityException("read only")
        return ParcelFileDescriptor.open(file(uri), ParcelFileDescriptor.MODE_READ_ONLY)
    }

    override fun getType(uri: Uri): String = mime(uri.lastPathSegment ?: "")

    override fun query(uri: Uri, projection: Array<out String>?, selection: String?, selectionArgs: Array<out String>?, sortOrder: String?): Cursor {
        val f = file(uri)
        val cols = projection ?: arrayOf(OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE)
        val c = MatrixCursor(cols)
        c.addRow(cols.map { col -> when (col) { OpenableColumns.DISPLAY_NAME -> f.name; OpenableColumns.SIZE -> f.length(); else -> null } })
        return c
    }

    override fun insert(uri: Uri, values: ContentValues?): Uri? = null
    override fun delete(uri: Uri, selection: String?, selectionArgs: Array<out String>?) = 0
    override fun update(uri: Uri, values: ContentValues?, selection: String?, selectionArgs: Array<out String>?) = 0

    companion object {
        const val AUTHORITY = "fm.jiant.stems.exports"
        fun dir(ctx: Context) = File(ctx.cacheDir, "exports").apply { mkdirs() }
        fun uri(f: File): Uri = Uri.Builder().scheme("content").authority(AUTHORITY).appendPath(f.name).build()
        fun mime(name: String) = if (name.endsWith(".zip")) "application/zip" else if (name.endsWith(".wav")) "audio/wav" else "application/octet-stream"
    }
}
