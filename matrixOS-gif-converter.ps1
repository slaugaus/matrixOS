# USAGE: .\matrixOS-gif-converter [input file] [output filename] (crop)
#   Input: Any* file supported by ffmpeg   (* probably)
#   Output: A GIF
#   crop: If present, crops the video to 3:2 instead of letter/pillarboxing it
#   Have ffmpeg, ffprobe, and gifski in your PATH.

# Gives a fraction...
$fpsFrac = ffprobe -v error -select_streams v -of default=noprint_wrappers=1:nokey=1 -show_entries stream=r_frame_rate $args[0]
# ...Evaluate that into a float (https://superuser.com/a/1784841)
$fps = [Data.DataTable]::New().Compute($fpsFrac, $null)

mkdir gifTmp

if ($args[2] -eq "crop"){
    ffmpeg -i $args[0] -vf "scale=192:128:force_original_aspect_ratio=increase,crop=192:128" .\gifTmp\%04d.png
} else {
    ffmpeg -i $args[0] -vf "scale=192:128:force_original_aspect_ratio=decrease,pad=192:128:(ow-iw)/2:(oh-ih)/2" .\gifTmp\%04d.png
}

gifski --fps $fps --output $args[1] .\gifTmp\*.png

Remove-Item .\gifTmp -Recurse -Force
