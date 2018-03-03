### (c)2000-2011 Robert Hegemann
###
### /<path>/<artist>/<year>-<album>/<track-no> <title>
###
### SRC_ROOT : where the CD are located
### DST_ROOT : where newly encoded tracks should go
### DONE_ROOT : where verified encoded tracks are located
### LAME_EXE : points to program
### LAME_OPT : what options to use
################################################################
_V=2
SRC_ROOT=/windows/W/CD
DST_ROOT=/windows/Z/mp3v${_V}wg
DONE_ROOT=/windows/Z/mp3v${_V}wg
LAME_EXE=lame-399
LAME_OPT="-V${_V} --quiet --noreplaygain --id3v2-only"

SKIP_DONE_FOLDER_EXISTS=1
SKIP_DEST_FOLDER_EXISTS=1
SCAN_ALBUM_GAIN_ONLY=0

case "$0" in
    mk_album_gain.sh) SCAN_ALBUM_GAIN_ONLY=1;;
esac 
case "$1" in
    --scan-gain-only) SCAN_ALBUM_GAIN_ONLY=1;;
esac
if [ $SCAN_ALBUM_GAIN_ONLY = 1 ]
then
    SKIP_DONE_FOLDER_EXISTS=0
    SKIP_DEST_FOLDER_EXISTS=0
fi

if (test -e "${SRC_ROOT}")
then
    if test -e "${DST_ROOT}"
    then
        :
    else
        mkdir "${DST_ROOT}"
    fi

    for _artist in "${SRC_ROOT}"/*
    do
        _ARTIST=$(basename "${_artist}")
        DST_ARTIST=${DST_ROOT}/${_ARTIST}
        DS2_ARTIST=${DONE_ROOT}/${_ARTIST}
        echo "${_ARTIST}"

        VARIOUS_ARTISTS_MODE=0
        case "${_ARTIST}" in
        "Various" | "Various Artists" )
            VARIOUS_ARTISTS_MODE=1
            ID3_ALBUM_ARTIST="Various Artists"
            ;;
        "Musik Express" )
            VARIOUS_ARTISTS_MODE=2
            ID3_ALBUM_ARTIST="Various (ME)"
            ;;
        *)
            VARIOUS_ARTISTS_MODE=0
            ID3_ALBUM_ARTIST=${_ARTIST}
            ;;
        esac

        for _cd in "${_artist}"/*
        do
            _CD=$(basename "${_cd}")
            DST_CD=${DST_ARTIST}/${_CD}
            DS2_CD=${DS2_ARTIST}/${_CD}
            ID3_YR=`echo "${_CD}"|cut -b 1-4`
            ID3_CD=`echo "${_CD}"|cut -b 6-`
            if [ $SKIP_DONE_FOLDER_EXISTS = 1 ]
            then
                if test -e "${DS2_CD}"
                then
                    continue
                fi
            fi
            if [ $SKIP_DEST_FOLDER_EXISTS = 1 ]
            then
                if test -e "${DST_CD}"
                then
                    continue
                fi
            fi
            echo "${_ARTIST} / ${_CD}"

            ALBUM_GAIN="1.0"
            if test -e "${_cd}/album_gain_scale.txt"
            then
                ALBUM_GAIN=`cat "${_cd}/album_gain_scale.txt"`
            elif test -e "${DS2_CD}/album_gain_scale.txt"
            then
                ALBUM_GAIN=`cat "${DS2_CD}/album_gain_scale.txt"`
            elif test -e "${DST_CD}/album_gain_scale.txt"
            then
                ALBUM_GAIN=`cat "${DST_CD}/album_gain_scale.txt"`
            else
                unset ALBUM_GAIN
            fi
            if [ "${ALBUM_GAIN}" = "" ]
            then
                if test -e "${DST_ARTIST}"
                then
                    :
                else
                    mkdir "${DST_ARTIST}"
                fi
                if test -e "${DST_CD}"
                then
                    :
                else
                    mkdir "${DST_CD}"
                fi
                ALBUM_GAIN=`wavegain -x -a "${_cd}"/ 2>/dev/null`
                echo ${ALBUM_GAIN} > "${DST_CD}/album_gain_scale.txt"
            fi
            if [ $SCAN_ALBUM_GAIN_ONLY = 1 ]
            then
                continue
            fi

            for _song in "${_cd}"/*.wav
            do
                _SONG=$(basename "${_song}" .wav)
                DST_SONG=${DST_CD}/$(basename "${_song}" .wav).mp3
                if test -e "${DST_SONG}"
                then
                    :
                else
                    if test -e "${DST_ARTIST}"
                    then
                        :
                    else
                        mkdir "${DST_ARTIST}"
                    fi
                    if test -e "${DST_CD}"
                    then
                        :
                    else
                        mkdir "${DST_CD}"
                    fi
                    ID3_TRACK=`echo "${_SONG}"|cut -b 1-2`
                    if [ $VARIOUS_ARTISTS_MODE = 0 ]
                    then
                        ID3_TITLE=`echo "${_SONG}"|cut -b 4-`
                        ID3_TRACK_ARTIST=${_ARTIST}
                    else
                        ID3_TITLE=`echo "${_SONG% - *}"|cut -b 4-`
                        ID3_TRACK_ARTIST=`echo "${_SONG#* - }"`
                    fi
                    ${LAME_EXE} ${LAME_OPT} \
                            --scale ${ALBUM_GAIN} \
                            --ta "${ID3_TRACK_ARTIST}" \
                            --tl "${ID3_CD}" \
                            --ty "${ID3_YR}" \
                            --tt "${ID3_TITLE}" \
                            --tn "${ID3_TRACK}" \
                            --tv "TXXX=ALBUM ARTIST=${ID3_ALBUM_ARTIST}" \
                            --tv "TXXX=LAME SCALE=${ALBUM_GAIN}" \
                            "${_song}" "${DST_SONG}" &
                fi
            done
            wait
        done
    done
else
    echo Quellverzeichnis ${SRC_ROOT} existiert nicht.
fi
