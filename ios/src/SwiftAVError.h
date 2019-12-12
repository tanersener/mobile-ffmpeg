// Exposes AVUtil error values to Swift.
// See libavutil/error.h for comments + reference.

#import <Foundation/Foundation.h>
#import <libavutil/error.h>

#if EDOM > 0

NS_INLINE __attribute__((swift_name("AVERROR(_:)"))) int _AVERROR(int errnum) {
    return AVERROR(errnum);
}

NS_INLINE __attribute__((swift_name("AVUNERROR(_:)"))) int _AVUNERROR(int errnum) {
    return AVUNERROR(errnum);
}

#else

NS_INLINE __attribute__((swift_name("AVERROR(_:)"))) int _AVERROR(int errnum) {
    return errnum;
}

NS_INLINE __attribute__((swift_name("AVUNERROR(_:)"))) int _AVUNERROR(int errnum) {
    return errnum;
}

#endif

NS_INLINE __attribute__((swift_name("AVERROR_BSF_NOT_FOUND()"))) int _AVERROR_BSF_NOT_FOUND(void) {
    return AVERROR_BSF_NOT_FOUND;
}

NS_INLINE __attribute__((swift_name("AVERROR_BUG()"))) int _AVERROR_BUG(void) {
    return AVERROR_BUG;
}

NS_INLINE __attribute__((swift_name("AVERROR_BUFFER_TOO_SMALL()"))) int _AVERROR_BUFFER_TOO_SMALL(void) {
    return AVERROR_BUFFER_TOO_SMALL;
}

NS_INLINE __attribute__((swift_name("AVERROR_DECODER_NOT_FOUND()"))) int _AVERROR_DECODER_NOT_FOUND(void) {
    return AVERROR_DECODER_NOT_FOUND;
}

NS_INLINE __attribute__((swift_name("AVERROR_DEMUXER_NOT_FOUND()"))) int _AVERROR_DEMUXER_NOT_FOUND(void) {
    return AVERROR_DEMUXER_NOT_FOUND;
}

NS_INLINE __attribute__((swift_name("AVERROR_ENCODER_NOT_FOUND()"))) int _AVERROR_ENCODER_NOT_FOUND(void) {
    return AVERROR_ENCODER_NOT_FOUND;
}

NS_INLINE __attribute__((swift_name("AVERROR_EOF()"))) int _AVERROR_EOF(void) {
    return AVERROR_EOF;
}

NS_INLINE __attribute__((swift_name("AVERROR_EXIT()"))) int _AVERROR_EXIT(void) {
    return AVERROR_EXIT;
}



NS_INLINE __attribute__((swift_name("AVERROR_EXTERNAL()"))) int _AVERROR_EXTERNAL(void) {
    return AVERROR_EXTERNAL;
}


NS_INLINE __attribute__((swift_name("AVERROR_FILTER_NOT_FOUND()"))) int _AVERROR_FILTER_NOT_FOUND(void) {
    return AVERROR_FILTER_NOT_FOUND;
}

NS_INLINE __attribute__((swift_name("AVERROR_INVALIDDATA()"))) int _AVERROR_INVALIDDATA(void) {
    return AVERROR_INVALIDDATA;
}


NS_INLINE __attribute__((swift_name("AVERROR_MUXER_NOT_FOUND()"))) int _AVERROR_MUXER_NOT_FOUND(void) {
    return AVERROR_MUXER_NOT_FOUND;
}

NS_INLINE __attribute__((swift_name("AVERROR_OPTION_NOT_FOUND()"))) int _AVERROR_OPTION_NOT_FOUND(void) {
    return AVERROR_OPTION_NOT_FOUND;
}

NS_INLINE __attribute__((swift_name("AVERROR_PATCHWELCOME()"))) int _AVERROR_PATCHWELCOME(void) {
    return AVERROR_PATCHWELCOME;
}



NS_INLINE __attribute__((swift_name("AVERROR_PROTOCOL_NOT_FOUND()"))) int _AVERROR_PROTOCOL_NOT_FOUND(void) {
    return AVERROR_PROTOCOL_NOT_FOUND;
}

NS_INLINE __attribute__((swift_name("AVERROR_STREAM_NOT_FOUND()"))) int _AVERROR_STREAM_NOT_FOUND(void) {
    return AVERROR_STREAM_NOT_FOUND;
}



NS_INLINE __attribute__((swift_name("AVERROR_BUG2()"))) int _AVERROR_BUG2(void) {
    return AVERROR_BUG2;
}

NS_INLINE __attribute__((swift_name("AVERROR_UNKNOWN()"))) int _AVERROR_UNKNOWN(void) {
    return AVERROR_UNKNOWN;
}


NS_INLINE __attribute__((swift_name("AVERROR_EXPERIMENTAL()"))) int _AVERROR_EXPERIMENTAL(void) {
    return AVERROR_EXPERIMENTAL;
}


NS_INLINE __attribute__((swift_name("AVERROR_INPUT_CHANGED()"))) int _AVERROR_INPUT_CHANGED(void) {
    return AVERROR_INPUT_CHANGED;
}

NS_INLINE __attribute__((swift_name("AVERROR_OUTPUT_CHANGED()"))) int _AVERROR_OUTPUT_CHANGED(void) {
    return AVERROR_OUTPUT_CHANGED;
}


NS_INLINE __attribute__((swift_name("AVERROR_HTTP_BAD_REQUEST()"))) int _AVERROR_HTTP_BAD_REQUEST(void) {
    return AVERROR_HTTP_BAD_REQUEST;
}

NS_INLINE __attribute__((swift_name("AVERROR_HTTP_UNAUTHORIZED()"))) int _AVERROR_HTTP_UNAUTHORIZED(void) {
    return AVERROR_HTTP_UNAUTHORIZED;
}

NS_INLINE __attribute__((swift_name("AVERROR_HTTP_FORBIDDEN()"))) int _AVERROR_HTTP_FORBIDDEN(void) {
    return AVERROR_HTTP_FORBIDDEN;
}

NS_INLINE __attribute__((swift_name("AVERROR_HTTP_NOT_FOUND()"))) int _AVERROR_HTTP_NOT_FOUND(void) {
    return AVERROR_HTTP_NOT_FOUND;
}

NS_INLINE __attribute__((swift_name("AVERROR_HTTP_OTHER_4XX()"))) int _AVERROR_HTTP_OTHER_4XX(void) {
    return AVERROR_HTTP_OTHER_4XX;
}

NS_INLINE __attribute__((swift_name("AVERROR_HTTP_SERVER_ERROR()"))) int _AVERROR_HTTP_SERVER_ERROR(void) {
    return AVERROR_HTTP_SERVER_ERROR;
}

NS_INLINE __attribute__((swift_name("av_err2str(_:)"))) char *_av_err2str(int errnum) {
    return av_err2str(errnum);
}
