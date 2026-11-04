// SPDX-License-Identifier: GPL-3.0-only
// JIANT Stems: every string the app shows, in English and Spanish (rioplatense, as the FM-1's manual). One enum,
// so a string cannot exist in one language and not the other; the tests check the format arguments match.
package fm.jiant.stems.core

import java.util.Locale

enum class T(val en: String, val es: String) {
    APP_SUB("Record the FM-1's stems, line them up, export", "Grabá los stems del FM-1, alinealos, exportá"),
    NO_DEVICE("No FM-1 found", "No encuentro el FM-1"),
    CONNECT_HINT("Plug the FM-1 into the phone with a USB-C data cable.", "Enchufá el FM-1 al teléfono con un cable USB-C de datos."),
    ON_FM1("ON THE FM-1", "EN EL FM-1"),
    HOW_STEPS("1  MENU › SYSTEM › USB › STEMS+IN (firmware 0.8.0 or later: the stems and your voice through the FM-1) or STEMS (the eight stems).\n2  Unplug the USB cable and plug it in again.\n3  Tap CONNECT and allow the USB access.",
              "1  MENU › SYSTEM › USB › STEMS+IN (firmware 0.8.0 o posterior: los stems y tu voz pasando por el FM-1) o STEMS (los ocho stems).\n2  Desenchufá el cable USB y volvé a enchufarlo.\n3  Tocá CONECTAR y permití el acceso USB."),
    FOUND("Found: %s", "Encontrado: %s"),
    MODE_STEMS("8 stems: T1–T6 and the FX return. For your voice through the FM-1 as well, use MENU › SYSTEM › USB › STEMS+IN.",
               "8 stems: T1–T6 y el retorno de FX. Para sumar tu voz pasando por el FM-1, usá MENU › SYSTEM › USB › STEMS+IN."),
    MODE_STEMS_IN("10 channels: T1–T6, the FX return, your voice through the last live track (VOX) and as it came (DRY).",
                  "10 canales: T1–T6, el retorno de FX, tu voz pasando por la última pista activa (VOX) y tal como llegó (DRY)."),
    MODE_STEREO("The FM-1 sends 2 channels (the stereo mix). For the stems set MENU › SYSTEM › USB › STEMS+IN or STEMS and replug. You can still record the mix.",
                "El FM-1 manda 2 canales (la mezcla estéreo). Para los stems poné MENU › SYSTEM › USB › STEMS+IN o STEMS y reenchufá. Igual podés grabar la mezcla."),
    MODE_OTHER("%d channels: each one recorded as its own lane.", "%d canales: cada uno se graba como un carril."),
    CONNECT("CONNECT", "CONECTAR"),
    RECORD("RECORD", "GRABAR"),
    SEARCH("SEARCH AGAIN", "BUSCAR DE NUEVO"),
    USB_DENIED("USB access was not allowed. Tap CONNECT again and accept it.", "No se permitió el acceso USB. Tocá CONECTAR de nuevo y aceptalo."),
    OPEN_FAILED("Android did not let the app open the FM-1. Unplug it, plug it in again and retry.", "Android no dejó abrir el FM-1. Desenchufalo, volvé a enchufarlo y probá de nuevo."),
    BAD_FORMAT("The FM-1's USB format is not one this app reads (%s).", "El formato USB del FM-1 no es uno que esta app lea (%s)."),
    TAKES("TAKES", "TOMAS"),
    NO_TAKES("No takes yet. Connect the FM-1 and record one.", "Todavía no hay tomas. Conectá el FM-1 y grabá una."),
    N_TAKES("%d TAKES", "%d TOMAS"),
    FREE("%s FREE", "%s LIBRES"),
    DELETE_TAKE("Delete “%s” and its files?", "¿Borrar “%s” y sus archivos?"),
    DELETE("DELETE", "BORRAR"),
    CANCEL("CANCEL", "CANCELAR"),
    OK("OK", "OK"),
    TAKE_NAME("TAKE %02d", "TOMA %02d"),

    REC("REC", "REC"),
    READY("READY", "LISTO"),
    RECORDING("RECORDING", "GRABANDO"),
    SAVING("Saving the take…", "Guardando la toma…"),
    ARM_HINT("Tap a meter to arm or disarm its lane. The armed lanes are recorded, each as its own WAV.",
             "Tocá un medidor para armar o desarmar su carril. Los carriles armados se graban, cada uno en su WAV."),
    MIC("MIC → FM-1", "MIC → FM-1"),
    MIC_GAIN("MIC GAIN", "GANANCIA MIC"),
    LATENCY("LATENCY", "LATENCIA"),
    MEASURE("MEASURE", "MEDIR"),
    MEASURING("Measuring for 6 s: no headphones, the phone about 20 cm from the FM-1's speaker, a song with drums playing, the last live track's LEVEL down or muted.",
              "Midiendo 6 s: sin auriculares, el teléfono a unos 20 cm del parlante del FM-1, un tema con batería sonando y la última pista activa con el LEVEL bajo o muteada."),
    MEASURED("Round trip %.1f ms (clarity %.1f). Saved for the next takes.", "Ida y vuelta %.1f ms (claridad %.1f). Queda guardada para las próximas tomas."),
    UNCLEAR("The measure was not clear (clarity %.1f). Drums louder, the phone closer to the speaker, and again.",
            "La medición no fue clara (claridad %.1f). Batería más fuerte, el teléfono más cerca del parlante, y otra vez."),
    MIC_NEEDS_STEMS_IN("Your voice through the FM-1 needs MENU › SYSTEM › USB › STEMS+IN (firmware 0.8.0).",
                       "Tu voz por el FM-1 necesita MENU › SYSTEM › USB › STEMS+IN (firmware 0.8.0)."),
    MIC_PERMISSION("The app needs the microphone permission to send your voice.", "La app necesita el permiso de micrófono para mandar tu voz."),
    MIC_FAILED("Could not open the phone's microphone.", "No pude abrir el micrófono del teléfono."),
    MIC_HINT("Your voice goes into the FM-1's last live track (its LEVEL, PAN and sends) and comes back as VOX. With headphones on the FM-1, no feedback.",
             "Tu voz entra a la última pista activa del FM-1 (su LEVEL, PAN y sends) y vuelve como VOX. Con auriculares en el FM-1, sin acople."),
    OLD_FIRMWARE("This FM-1's firmware (0.9.7 or older) loses audio in STEMS: clicks in every stem. Update the FM-1 to 0.9.8 or later.",
                 "El firmware de este FM-1 (0.9.7 o anterior) pierde audio en STEMS: clics en todos los stems. Actualizá el FM-1 a 0.9.8 o posterior."),
    STREAM_ERROR("The USB stream stopped (error %d). Unplug the FM-1 and plug it in again.", "Se cortó el audio USB (error %d). Desenchufá el FM-1 y volvé a enchufarlo."),
    DISK_ERROR("Could not write the take: %s", "No pude escribir la toma: %s"),
    NOTHING_ARMED("Arm at least one lane.", "Armá al menos un carril."),
    DETACHED("The FM-1 was unplugged.", "Se desenchufó el FM-1."),
    LAT_NONE("not measured", "sin medir"),
    LAT_TAKE("measured in the take", "medida en la toma"),
    LAT_CAL("from the last measure", "de la última medición"),
    LAT_MANUAL("set by hand", "a mano"),
    LOW_SPACE("Little space left on the phone: %s.", "Queda poco espacio en el teléfono: %s."),

    EDIT("EDIT", "EDITAR"),
    LOADING("Reading the take…", "Leyendo la toma…"),
    GAIN("GAIN", "GANANCIA"),
    PAN("PAN", "PANEO"),
    SHIFT("SHIFT", "CORRER"),
    TRIM_IN("SET IN", "INICIO AQUÍ"),                 // (1.1: "INICIO" / "FINAL" read as "go to the start / end")
    TRIM_OUT("SET OUT", "FINAL AQUÍ"),
    TRIM_ALL("FULL", "SIN RECORTE"),
    RENAME("RENAME", "RENOMBRAR"),
    TAKE_NAME_TITLE("Take name", "Nombre de la toma"),
    EDIT_HINT("Tap a lane to select it, M mutes it, S solos it. Drag a card to change it, double-tap it to reset. Drag the orange edges to trim, tap the ruler to move the playhead.",
              "Tocá un carril para elegirlo, M lo mutea, S lo deja solo. Arrastrá una tarjeta para cambiarla, doble toque para volverla a cero. Arrastrá los bordes naranjas para recortar, tocá la regla para mover el cabezal."),
    LAT_INFO("Voice moved %.1f ms earlier (%s).", "Voz corrida %.1f ms antes (%s)."),
    LANE_MISSING("%s: file missing", "%s: falta el archivo"),

    EXPORT("EXPORT", "EXPORTAR"),
    STEMS_ZIP("STEMS ZIP", "STEMS ZIP"),
    MERGE("MERGE", "MEZCLA"),
    FORMAT("FORMAT", "FORMATO"),
    WAV_BITS("WAV %d bit", "WAV %d bit"),
    RATE("RATE", "FRECUENCIA"),
    STEMS("STEMS", "STEMS"),
    AS_RECORDED("As recorded", "Como se grabaron"),
    WITH_MIX("With the mix", "Con la mezcla"),
    ADD_MIX("ADD MIX.wav", "SUMAR MIX.wav"),
    NORMALIZE("NORMALIZE", "NORMALIZAR"),
    YES("YES", "SÍ"),
    NO("NO", "NO"),
    OFF("OFF", "OFF"),
    ON("ON", "ON"),
    SAVE("SAVE TO FILES", "GUARDAR EN ARCHIVOS"),
    SHARE("SHARE", "COMPARTIR"),
    EXPORTING("Exporting… %d%%", "Exportando… %d%%"),
    SAVED("Saved: %s", "Guardado: %s"),
    EXPORT_FAILED("Export failed: %s", "Falló la exportación: %s"),
    SHARE_TITLE("Share the take", "Compartir la toma"),
    EXPORT_FILES("%d files, %s", "%d archivos, %s"),
    EXPORT_HINT_ZIP("One WAV a lane, trimmed, the voice already moved by the latency.", "Un WAV por carril, recortado, con la voz ya corrida por la latencia."),
    EXPORT_TRIMMED("Trimmed: %s of the take's %s. To export it all: EDIT › FULL.",
                   "Recortada: %s de los %s de la toma. Para exportarla entera: EDITAR › SIN RECORTE."),
    EXPORT_HINT_MIX("The stereo mix as you hear it in EDIT: gains, pans, mutes, the trim.", "La mezcla estéreo como suena en EDITAR: ganancias, paneos, mutes, el recorte."),
    ;

    /** the string in the app's language, formatted with [args] */
    fun s(vararg args: Any?): String {
        val t = if (Lang.spanish) es else en
        return if (args.isEmpty()) t else String.format(Lang.locale, t, *args)
    }
}

object Lang {
    @Volatile var spanish = false
    val locale: Locale get() = if (spanish) Locale.forLanguageTag("es") else Locale.ENGLISH

    /** Spanish for a Spanish-speaking device, English otherwise */
    fun init(language: String) { spanish = language.lowercase(Locale.ROOT) == "es" }
}
