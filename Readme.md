MobileFFmpeg Colaboradores financieros en Open Collective Lanzamiento de GitHub Bintray CocoaPods Estado de construcción
FFmpeg para Android, iOS y tvOS



1. Características
Incluye ambos FFmpegyFFprobe

Utilice los binarios disponibles en Github/ JCenter/ CocoaPodso cree su propia versión con las bibliotecas externas que necesite

Apoyos

Android, iOS y tvOS

FFmpeg v3.4.x, v4.0.x, v4.1, v4.2, v4.3y v4.4-devcomunicados

29 bibliotecas externas

chromaprint, fontconfig, freetype, fribidi, gmp, gnutls, kvazaar, lame, libaom, libass, libiconv, libilbc, libtheora, libvorbis, libvpx, libwebp, libxml2, opencore-amr, openh264, opus, sdl, shine, snappy, soxr, speex, tesseract, twolame, vo-amrwbenc,wavpack

5 bibliotecas externas con licencia GPL

rubberband, vid.stab, x264, x265,xvidcore

Ejecución concurrente

Expone las capacidades de la biblioteca FFmpeg y de la biblioteca contenedora MobileFFmpeg

Incluye instrucciones de compilación cruzada para 47 bibliotecas de código abierto

chromaprint, expat, ffmpeg, fontconfig, freetype, fribidi, giflib, gmp, gnutls, kvazaar, lame, leptonica, libaom, libass, libiconv, libilbc, libjpeg, libjpeg-turbo, libogg, libpng, libsamplerate, libsndfile, libtheora, libuuid, libvorbis, libvpx, libwebp, libxml2, nettle, opencore-amr, openh264, opus, rubberband, sdl, shine, snappy, soxr, speex, tesseract, tiff, twolame, vid.stab, vo-amrwbenc, wavpack, x264, x265,xvidcore

Licencia bajo LGPL 3.0, se puede personalizar para admitir GPL v3.0

1.1 Android
Construye arm-v7a, arm-v7a-neon, arm64-v8a, x86y x86_64arquitecturas
Soportes zliby MediaCodecbibliotecas del sistema
Acceso a la cámara en dispositivos compatibles
Crea bibliotecas nativas compartidas (.so)
Crea un archivo de Android con la extensión .aar
Apoyos API Level 16+
1.2 iOS
Construye armv7, armv7s, arm64, arm64e, i386, x86_64y x86_64arquitecturas (Mac catalizador)
Soportes bzip2, iconv, libuuid, zliblibrerías del sistema y AudioToolbox, VideoToolbox, AVFoundationmarcos del sistema
API de Objective-C
Acceso a la cámara
ARC biblioteca habilitada
Construido con -fembed-bitcodebandera
Crea marcos estáticos, xcframeworks estáticos y bibliotecas estáticas universales (fat) (.a)
Apoya iOS SDK 9.3o posterior
1.3 tvOS
Construcciones arm64y x86_64arquitecturas
Soportes bzip2, iconv, libuuid, zliblibrerías del sistema y AudioToolbox, VideoToolboxmarcos del sistema
API de Objective-C
ARC biblioteca habilitada
Construido con -fembed-bitcodebandera
Crea marcos estáticos y bibliotecas estáticas universales (fat) (.a)
Apoya tvOS SDK 9.2o posterior
2. Usando
Los binarios prediseñados están disponibles en Github , JCenter y CocoaPods .

2.1 Paquetes
Hay ocho mobile-ffmpegpaquetes diferentes . A continuación, puede ver qué bibliotecas del sistema y bibliotecas externas están habilitadas en cada una de ellas.

Recuerde que algunas partes de FFmpegestán autorizadas GPLy solo GPLlos mobile-ffmpegpaquetes con licencia las incluyen.

min	min-gpl	https	https-gpl	audio	vídeo	completo	gpl completo
bibliotecas externas	-	vid.stab
x264
x265
xvidcore	gmp
gnutls	gmp
gnutls
vid.stab
x264
x265
xvidcore	cojo
libilbc
libvorbis
opencore-amr
opus
brillar
soxr
speex
twolame
vo-amrwbenc
wavpack	fontconfig
freetype
fribidi
kvazaar
libaom
libass
libiconv
libtheora
libvpx
libwebp
ágil	fontconfig
freetype
fribidi
gmp
gnutls
kvazaar
lame
libaom
libass
libiconv
libilbc
libtheora
libvorbis
libvpx
libwebp
libxml2
opencore-amr
opus
shine
snappy
soxr
speex
twolame
vo-amrwbenc
wavpack	fontconfig
freetype
fribidi
gmp
gnutls
kvazaar
cojo
libaom
libass
libiconv
libilbc
libtheora
libvorbis
libvpx
libwebp
libxml2
opencore-amr
opus
brillar
ágil
soxr
speex
twolame
vid.stab
vo-amrwbenc
wavpack
x264
x265
xvidcore
bibliotecas del sistema android	zlib
MediaCodec
bibliotecas del sistema ios	zlib
AudioToolbox
AVFoundation
iconv
VideoToolbox
bzip2
bibliotecas del sistema tvos	zlib
AudioToolbox
iconv
VideoToolbox
bzip2
libilbc, opus, snappy, x264Y xvidcoreson apoyados desdev1.1

libaomy soxrson compatibles desdev2.0

chromaprint, vid.staby x265son compatibles desdev2.1

sdl, tesseract, twolameBibliotecas externas; zlib, MediaCodecBibliotecas del sistema Android; bzip2, zlibBibliotecas de sistema iOS y AudioToolbox, VideoToolbox, AVFoundationmarcos sistema IOS son soportados, yav3.0

Dado que v4.2, chromaprint, sdly tesseractlas bibliotecas no se incluyen en las versiones binarias. Aún puede crearlos e incluirlos en sus lanzamientos

AVFoundationno está disponible en tvOS, VideoToolboxno está disponible en tvOSversiones LTS

Desde v4.3.1, iOSy las tvOSversiones comenzaron a usar la iconvbiblioteca del sistema en lugar de iconvla biblioteca externa

vo-amrwbenc es compatible desde v4.4

2.2 Android
Agregue la dependencia de MobileFFmpeg a su patrón build.gradlein mobile-ffmpeg-<package name>.

dependencies {
    implementation 'com.arthenica:mobile-ffmpeg-full:4.4'
}
Ejecute comandos síncronos de FFmpeg.

import com.arthenica.mobileffmpeg.Config;
import com.arthenica.mobileffmpeg.FFmpeg;

int rc = FFmpeg.execute("-i file1.mp4 -c:v mpeg4 file2.mp4");

if (rc == RETURN_CODE_SUCCESS) {
    Log.i(Config.TAG, "Command execution completed successfully.");
} else if (rc == RETURN_CODE_CANCEL) {
    Log.i(Config.TAG, "Command execution cancelled by user.");
} else {
    Log.i(Config.TAG, String.format("Command execution failed with rc=%d and the output below.", rc));
    Config.printLastCommandOutput(Log.INFO);
}
Ejecute comandos asincrónicos de FFmpeg.

import com.arthenica.mobileffmpeg.Config;
import com.arthenica.mobileffmpeg.FFmpeg;

long executionId = FFmpeg.executeAsync("-i file1.mp4 -c:v mpeg4 file2.mp4", new ExecuteCallback() {

    @Override
    public void apply(final long executionId, final int returnCode) {
        if (returnCode == RETURN_CODE_SUCCESS) {
            Log.i(Config.TAG, "Async command execution completed successfully.");
        } else if (returnCode == RETURN_CODE_CANCEL) {
            Log.i(Config.TAG, "Async command execution cancelled by user.");
        } else {
            Log.i(Config.TAG, String.format("Async command execution failed with returnCode=%d.", returnCode));
        }
    }
});
Ejecute los comandos FFprobe.

import com.arthenica.mobileffmpeg.Config;
import com.arthenica.mobileffmpeg.FFprobe;

int rc = FFprobe.execute("-i file1.mp4");

if (rc == RETURN_CODE_SUCCESS) {
    Log.i(Config.TAG, "Command execution completed successfully.");
} else {
    Log.i(Config.TAG, String.format("Command execution failed with rc=%d and the output below.", rc));
    Config.printLastCommandOutput(Log.INFO);
}
Verifique el resultado de la ejecución más tarde.

int rc = Config.getLastReturnCode();

if (rc == RETURN_CODE_SUCCESS) {
    Log.i(Config.TAG, "Command execution completed successfully.");
} else if (rc == RETURN_CODE_CANCEL) {
    Log.i(Config.TAG, "Command execution cancelled by user.");
} else {
    Log.i(Config.TAG, String.format("Command execution failed with rc=%d and the output below.", rc));
    Config.printLastCommandOutput(Log.INFO);
}
Detenga las operaciones de FFmpeg en curso.

Detener todas las ejecuciones
FFmpeg.cancel();
Detener una ejecución específica
FFmpeg.cancel(executionId);
Obtenga información multimedia para un archivo.

MediaInformation info = FFprobe.getMediaInformation("<file path or uri>");
Grabe video usando la cámara de Android.

FFmpeg.execute("-f android_camera -i 0:0 -r 30 -pixel_format bgr0 -t 00:00:05 <record file path>");
Habilitar registro de devolución de llamada.

Config.enableLogCallback(new LogCallback() {
    public void apply(LogMessage message) {
        Log.d(Config.TAG, message.getText());
    }
});
Habilite la devolución de llamada de estadísticas.

Config.enableStatisticsCallback(new StatisticsCallback() {
    public void apply(Statistics newStatistics) {
        Log.d(Config.TAG, String.format("frame: %d, time: %d", newStatistics.getVideoFrameNumber(), newStatistics.getTime()));
    }
});
Ignore el manejo de una señal.

Config.ignoreSignal(Signal.SIGXCPU);
Enumere las ejecuciones en curso.

final List<FFmpegExecution> ffmpegExecutions = FFmpeg.listExecutions();
for (int i = 0; i < ffmpegExecutions.size(); i++) {
    FFmpegExecution execution = ffmpegExecutions.get(i);
    Log.d(TAG, String.format("Execution %d = id:%d, startTime:%s, command:%s.", i, execution.getExecutionId(), execution.getStartTime(), execution.getCommand()));
}
Establecer el nivel de registro predeterminado.

Config.setLogLevel(Level.AV_LOG_FATAL);
Registre el directorio de fuentes personalizadas.

Config.setFontDirectory(this, "<folder with fonts>", Collections.EMPTY_MAP);
2.3 iOS / tvOS
Agregue la dependencia de MobileFFmpeg a su patrón Podfilein mobile-ffmpeg-<package name>.

iOS
pod 'mobile-ffmpeg-full', '~> 4.4'
tvOS
pod 'mobile-ffmpeg-tvos-full', '~> 4.4'
Ejecute comandos síncronos de FFmpeg.

#import <mobileffmpeg/MobileFFmpegConfig.h>
#import <mobileffmpeg/MobileFFmpeg.h>

int rc = [MobileFFmpeg execute: @"-i file1.mp4 -c:v mpeg4 file2.mp4"];

if (rc == RETURN_CODE_SUCCESS) {
    NSLog(@"Command execution completed successfully.\n");
} else if (rc == RETURN_CODE_CANCEL) {
    NSLog(@"Command execution cancelled by user.\n");
} else {
    NSLog(@"Command execution failed with rc=%d and output=%@.\n", rc, [MobileFFmpegConfig getLastCommandOutput]);
}
Ejecute comandos asincrónicos de FFmpeg.

#import <mobileffmpeg/MobileFFmpegConfig.h>
#import <mobileffmpeg/MobileFFmpeg.h>

long executionId = [MobileFFmpeg executeAsync:@"-i file1.mp4 -c:v mpeg4 file2.mp4" withCallback:self];

- (void)executeCallback:(long)executionId :(int)returnCode {
    if (rc == RETURN_CODE_SUCCESS) {
        NSLog(@"Async command execution completed successfully.\n");
    } else if (rc == RETURN_CODE_CANCEL) {
        NSLog(@"Async command execution cancelled by user.\n");
    } else {
        NSLog(@"Async command execution failed with rc=%d.\n", rc);
    }
}
Ejecute los comandos FFprobe.

#import <mobileffmpeg/MobileFFmpegConfig.h>
#import <mobileffmpeg/MobileFFprobe.h>

int rc = [MobileFFprobe execute: @"-i file1.mp4"];

if (rc == RETURN_CODE_SUCCESS) {
    NSLog(@"Command execution completed successfully.\n");
} else if (rc == RETURN_CODE_CANCEL) {
    NSLog(@"Command execution cancelled by user.\n");
} else {
    NSLog(@"Command execution failed with rc=%d and output=%@.\n", rc, [MobileFFmpegConfig getLastCommandOutput]);
}
Verifique el resultado de la ejecución más tarde.

int rc = [MobileFFmpegConfig getLastReturnCode];
NSString *output = [MobileFFmpegConfig getLastCommandOutput];

if (rc == RETURN_CODE_SUCCESS) {
    NSLog(@"Command execution completed successfully.\n");
} else if (rc == RETURN_CODE_CANCEL) {
    NSLog(@"Command execution cancelled by user.\n");
} else {
    NSLog(@"Command execution failed with rc=%d and output=%@.\n", rc, output);
}
Detenga las operaciones de FFmpeg en curso.

Detener todas las ejecuciones
[MobileFFmpeg cancel];

Detener una ejecución específica
[MobileFFmpeg cancel:executionId];
Obtenga información multimedia para un archivo.

MediaInformation *mediaInformation = [MobileFFprobe getMediaInformation:@"<file path or uri>"];
Grabe video y audio usando la cámara iOS. Esta operación no se admite en tvOSya AVFoundationque no está disponible en tvOS.

[MobileFFmpeg execute: @"-f avfoundation -r 30 -video_size 1280x720 -pixel_format bgr0 -i 0:0 -vcodec h264_videotoolbox -vsync 2 -f h264 -t 00:00:05 %@", recordFilePath];
Habilitar registro de devolución de llamada.

[MobileFFmpegConfig setLogDelegate:self];

- (void)logCallback:(long)executionId :(int)level :(NSString*)message {
    dispatch_async(dispatch_get_main_queue(), ^{
        NSLog(@"%@", message);
    });
}
Habilite la devolución de llamada de estadísticas.

[MobileFFmpegConfig setStatisticsDelegate:self];

- (void)statisticsCallback:(Statistics *)newStatistics {
    dispatch_async(dispatch_get_main_queue(), ^{
        NSLog(@"frame: %d, time: %d\n", newStatistics.getVideoFrameNumber, newStatistics.getTime);
    });
}
Ignore el manejo de una señal.

[MobileFFmpegConfig ignoreSignal:SIGXCPU];
Enumere las ejecuciones en curso.

NSArray* ffmpegExecutions = [MobileFFmpeg listExecutions];
for (int i = 0; i < [ffmpegExecutions count]; i++) {
    FFmpegExecution* execution = [ffmpegExecutions objectAtIndex:i];
    NSLog(@"Execution %d = id: %ld, startTime: %@, command: %@.\n", i, [execution getExecutionId], [execution getStartTime], [execution getCommand]);
}
Establecer el nivel de registro predeterminado.

[MobileFFmpegConfig setLogLevel:AV_LOG_FATAL];
Registre el directorio de fuentes personalizadas.

[MobileFFmpegConfig setFontDirectory:@"<folder with fonts>" with:nil];
2.4 Instalación manual
2.4.1 Android
Puede importar MobileFFmpegpaquetes aar Android Studiousando el menú File-> New-> New Module-> Import .JAR/.AAR Package.

2.4.2 iOS / tvOS
Los frameworks iOS y tvOS se pueden instalar manualmente usando la guía Importing Frameworks . Si desea utilizar binarios universales, consulte la guía Uso de binarios universales .

2.5 Aplicación de prueba
Puede ver cómo se usa MobileFFmpeg dentro de una aplicación ejecutando las aplicaciones de prueba proporcionadas. Hay una Androidaplicación de prueba debajo de la android/test-appcarpeta, una iOSaplicación de prueba debajo de la ios/test-appcarpeta y una tvOSaplicación de prueba debajo de la tvos/test-appcarpeta.

Todas las aplicaciones son idénticas y admiten ejecución de comandos, codificación de video, acceso a https, codificación de audio, grabación de subtítulos, estabilización de video, operaciones de canalización y ejecución de comandos concurrentes.



3. Versiones
MobileFFmpegel número de versión está alineado con FFmpegdesde la versión 4.2.

En versiones anteriores, la MobileFFmpegversión de un lanzamiento y la FFmpegversión incluida en ese lanzamiento eran diferentes. La siguiente tabla enumera las FFmpegversiones utilizadas en las MobileFFmpegversiones.

devparte FFmpegdel número de versión indica que la FFmpegfuente se extrae de la FFmpeg masterrama. El número de versión exacto se obtiene usando git describe --tags.
Versión de MobileFFmpeg	Versión FFmpeg	Fecha de lanzamiento
4.4	4.4-dev-416	25 de julio de 2020
4.4.LTS	4.4-dev-416	24 de julio de 2020
4.3.2	4.3-dev-2955	15 de abril de 2020
4.3.1	4.3-dev-1944	25 de enero de 2020
4.3.1.LTS	4.3-dev-1944	25 de enero de 2020
4.3	4.3-dev-1181	27 de oct de 2019
4.2.2	4.2-dev-1824	3 de julio de 2019
4.2.2.LTS	4.2-dev-1824	3 de julio de 2019
4.2.1	4.2-dev-1156	2 de abril de 2019
4.2	4.2-dev-480	3 de enero de 2019
4.2.LTS	4.2-dev-480	3 de enero de 2019
3.1	4.1-10	11 de diciembre de 2018
3,0	4.1-dev-1517	25 de octubre de 2018
2.2	4.0.3	10 de noviembre de 2018
2.1.1	4.0.2	19 de septiembre de 2018
2.1	4.0.2	5 de septiembre de 2018
2.0	4.0.1	30 de junio de 2018
1.2	3.4.4	30 de agosto de 2018
1.1	3.4.2	18 de junio de 2018
1.0	3.4.2	6 de junio de 2018
4. Versiones de LTS
A partir de v4.2, los MobileFFmpegbinarios se publican en dos variantes diferentes: Main Releasey LTS Release.

Las versiones principales incluyen la funcionalidad completa de la biblioteca y admiten las últimas funciones de SDK / API.

Las versiones de LTS están personalizadas para admitir una gama más amplia de dispositivos. Están construidos con versiones anteriores de API / SDK, por lo que algunas funciones no están disponibles en ellos.

Esta tabla muestra las diferencias entre dos variantes.

Lanzamiento principal	Lanzamiento de LTS
Nivel de API de Android	24	dieciséis
Acceso a la cámara de Android	si	-
Arquitecturas de Android	brazo-v7a-neón
arm64-v8a
x86
x86-64	brazo-v7a
brazo-v7a-neón
arm64-v8a
x86
x86-64
Soporte de Xcode	10.1	7.3.1
SDK de iOS	12,1	9.3
iOS AVFoundation	si	-
Arquitecturas iOS	arm64
arm64e 1
x86-64
x86-64-mac-catalizador 2	armv7
arm64
i386
x86-64
tvOS SDK	10,2	9.2
Arquitecturas tvOS	arm64
x86-64	arm64
x86-64
1 - Incluido hastav4.3.2

2 - Incluido desdev4.3.2

5. Edificio
Los scripts de compilación mastery las developmentramas se prueban periódicamente. Consulte el estado más reciente en la tabla siguiente.

rama	estado
Maestro	Estado de construcción
desarrollo	Estado de construcción
5.1 Prerrequisitos
Utilice su administrador de paquetes (apt, yum, dnf, brew, etc.) para instalar los siguientes paquetes.

autoconf automake libtool pkg-config curl cmake gcc gperf texinfo yasm nasm bison autogen patch git
Algunos de estos paquetes no son obligatorios para la compilación predeterminada. Visite Requisitos previos de Android , Requisitos previos de iOS y Requisitos previos de tvOS para obtener más detalles.

Las compilaciones de Android requieren estos paquetes adicionales.

Android SDK 4.1 Jelly Bean (API nivel 16) o posterior
Android NDK r21 o posterior con LLDB y CMake
Las compilaciones de iOS necesitan estos paquetes y herramientas adicionales.

Xcode 7.3.1 o posterior
iOS SDK 9.3 o posterior
Herramientas de línea de comandos
Las compilaciones de tvOS necesitan estos paquetes y herramientas adicionales.

Xcode 7.3.1 o posterior
tvOS SDK 9.2 o posterior
Herramientas de línea de comandos
5.2 Construir Scripts
Uso android.sh, ios.shy tvos.shpara construir MobileFFmpeg para cada plataforma.

Los tres scripts admiten opciones adicionales y se pueden personalizar para habilitar / deshabilitar bibliotecas y / o arquitecturas externas específicas. Consulte las páginas wiki de android.sh , ios.sh y tvos.sh para ver todas las opciones de compilación disponibles.

5.2.1 Android
export ANDROID_HOME=<Android SDK Path>
export ANDROID_NDK_ROOT=<Android NDK Path>
./android.sh


5.2.2 iOS
./ios.sh


5.2.3 tvOS
./tvos.sh


5.2.4 Creación de binarios LTS
Use la --ltsopción para construir sus binarios para cada plataforma.

5.3 Generar salida
Todas las bibliotecas creadas por los scripts de compilación de nivel superior ( android.sh, ios.shy tvos.sh) se pueden encontrar en el prebuiltdirectorio.

Androidarchivo (archivo .aar) se encuentra en la android-aarcarpeta
iOSlos frameworks se encuentran en la ios-frameworkcarpeta
iOSxcframeworks se encuentran en la ios-xcframeworkcarpeta
iOSlos binarios universales se encuentran en la ios-universalcarpeta
tvOSlos frameworks se encuentran en la tvos-frameworkcarpeta
tvOSlos binarios universales se encuentran en la tvos-universalcarpeta
5.4 Soporte GPL
Es posible habilitar bibliotecas con licencia GPL x264, xvidcoreya que v1.1; vid.stab, x265desde v2.1y rubberbanddesde v4.3.2los scripts de compilación de nivel superior. Su código fuente no se incluye en el repositorio y se descarga cuando está habilitado.

5.5 Bibliotecas externas
buildEl directorio incluye scripts de compilación de todas las bibliotecas externas. Existen dos scripts para cada biblioteca externa, uno para Androidy otro para iOS / tvOS. Cada uno de estos dos scripts contiene opciones / indicadores que se utilizan para realizar una compilación cruzada de la biblioteca en la plataforma móvil especificada.

Las optimizaciones de CPU ( ASM) están habilitadas para la mayoría de las bibliotecas externas. Los detalles y las excepciones se pueden encontrar en la página wiki de soporte de ASM .

6. Documentación
Una documentación más detallada está disponible en Wiki .

7. Contribuyentes
7.1 Colaboradores del código
Este proyecto existe gracias a todas las personas que contribuyen. [ Contribuir ]. 

7.2 Contribuyentes financieros
Conviértete en un contribuyente financiero y ayúdanos a sostener nuestra comunidad. [ Contribuir ]

7.2.1 Individuos


7.2.2 Organizaciones
Apoye este proyecto con su organización. Su logotipo aparecerá aquí con un enlace a su sitio web. [ Contribuir ]

         

8. Licencia
MobileFFmpegtiene licencia bajo LGPL v3.0. Sin embargo, si el código fuente se crea con el --enable-gplindicador opcional o -gplse utilizan binarios precompilados con postfix, MobileFFmpeg está sujeto a la licencia GPL v3.0.

El código fuente de todas las bibliotecas externas incluidas cumple con sus licencias individuales.

openh264El código fuente incluido en este repositorio tiene la licencia BSD de 2 cláusulas, pero esta licencia no cubre las MPEG LAtarifas de licencia. Si construye mobile-ffmpegcon openh264esa biblioteca y la distribuye, entonces está sujeto a pagar MPEG LAtarifas de licencia. Consulte la página de preguntas frecuentes de OpenH264 para obtener más detalles. Tenga en cuenta que mobile-ffmpegno publica un binario con openh264inside.

strip-frameworks.shEl script incluido y distribuido (hasta v4.x) se publica bajo la licencia Apache versión 2.0 .

En aplicaciones de prueba; Las fuentes incrustadas tienen licencia de SIL Open Font License , otros activos digitales se publican en el dominio público.

Visite la página de Licencia para obtener más detalles.

9. Patentes
No se explica claramente en su documentación, pero se cree que FFmpeg, kvazaar, x264y x265 incluye algoritmos que están sujetos a las patentes de software. Si vive en un país donde los algoritmos de software son patentables, probablemente deba pagar regalías a los titulares de patentes. Sin embargo, no somos abogados, por lo que le recomendamos que busque asesoramiento legal primero. Consulte las minipreguntas frecuentes sobre patentes de FFmpeg .

openh264establece claramente que utiliza algoritmos patentados. Por lo tanto, si crea mobile-ffmpeg con openh264 y distribuye esa biblioteca, estará sujeto a pagar tarifas de licencia de MPEG LA. Consulte la página de preguntas frecuentes de OpenH264 para obtener más detalles.

10. Contribuir
No dude en enviar problemas o solicitudes de extracción.

Tenga en cuenta que la masterrama incluye solo el último código fuente publicado. Los cambios previstos para la próxima versión se implementan en la developmentrama. Por lo tanto, si desea crear una solicitud de extracción, ábrala contra el development.

11. Ver también
preprocesador de gas libav
Documentación de la API de FFmpeg
Wiki FFmpeg
Licencias de biblioteca externa de FFmpeg
