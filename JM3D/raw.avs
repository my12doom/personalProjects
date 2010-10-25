LoadPlugin("JM3D_Source")
tb = JM3DSource(212)
l = crop(tb, 0, 0, 0, tb.Height/2)
r = crop(tb, 0, tb.height/2, 0, 0)
return StackHorizontal(l,r)


# parameters:
# JM3DSource(frame_count, [width], [height], [buffer_count], [views])
# default value: width = 1920, height = 1080