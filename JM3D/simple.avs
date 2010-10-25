LoadPlugin("JM3D_Source")
tb = JM3DSource2("00001.ssif")
l = crop(tb, 0, 0, 0, tb.Height/2)
r = crop(tb, 0, tb.height/2, 0, 0)
return StackHorizontal(l,r)


# parameters:
# JM3DSource2(file1, [file2], [views], [buffer_count], [left_pid], [right_pid])
# raw stream NOT supported, use JM3DSource for raw stream.
# views : 0=left, 1=right, 2=top-bottom(default)