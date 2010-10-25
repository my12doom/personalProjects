set input1="00001.ssif"
'set input2="00002.m2ts"
set LR=1
set output="pan.mkv"
set bitrate=2000
'set size=480

'AVS generation
del go.avs

echo LoadPlugin("JM3D_Source") >> go.avs

if defined input2 echo tb = JM3DSource2(%input1%, %input2%) >> go.avs
if not defined input2 echo tb = JM3DSource2(%input1%) >> go.avs

if %LR%==1 echo l = crop(tb, 0, 0, 0, tb.Height/2) >> go.avs
if %LR%==1 echo r = crop(tb, 0, tb.height/2, 0, 0) >> go.avs
if %LR%==1 echo o = StackHorizontal(l,r) >> go.avs
if not %LR%==1 echo o = tb >> go.avs

if defined size set resizestr=return o.LanczosResize(%size%*o.Width/o.Height/4*4,%size%)
if not defined size set resizestr=return o
echo %resizestr% >> go.avs

'x264
del *.temp
del stats.*
set program=x264
set input="go.avs"

#%program% --profile high --pass 1 --bitrate %bitrate% --stats "stats" --output NUL %input%
#%program% --profile high --pass 2 --bitrate %bitrate% --stats "stats" --output %output% %input%
%program% --bitrate %bitrate% --output %output% %input%

del go.avs
del stats.*
del *.yuv
pause