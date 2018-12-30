#
# Patch for issue https://github.com/tanersener/mobile-ffmpeg/issues/18
#

# Required environment variables
#
# BUILT_PRODUCTS_DIR
# FRAMEWORKS_FOLDER_PATH

echo "Applying patch for https://github.com/tanersener/mobile-ffmpeg/issues/18"
cd "${BUILT_PRODUCTS_DIR}/${FRAMEWORKS_FOLDER_PATH}"

INFO_PLIST_FILES=$(find . -name Info.plist | grep 'libswresample\|libavfilter\|libavformat\|libavutil\|libswscale\|libavdevice\|libavcodec\|mobileffmpeg')

for INFO_PLIST in ${INFO_PLIST_FILES};
do
        existing_key_line_number=$(sed -n '/MinimumOS/=' ${INFO_PLIST})
        if [[ -z ${existing_key_line_number} ]]; then
            sed -i .tmp "s/<\/dict/    <key>MinimumOSVersion<\/key>\\
    <string>9.3<\/string>\\
<\/dict/g" ${INFO_PLIST}
            rm -f "${INFO_PLIST}.tmp"
            echo "Patch added MinimumOSVersion key & value at ${INFO_PLIST}"
        else
            sed -i .tmp "$((existing_key_line_number+1))s/.*/        <string>9.3<\/string>/" ${INFO_PLIST}
            rm -f "${INFO_PLIST}.tmp"
            echo "Patch replaced MinimumOSVersion value at ${INFO_PLIST}"
        fi
done