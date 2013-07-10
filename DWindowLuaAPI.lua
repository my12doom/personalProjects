dx9 = {
sucess = draw_font_core(font, color)
resource, width, height = load_bitmap_core(filename)
nil = commit_resource_core(resource)
nil = decommit_resource_core(resource)
nil = release_resource_core(resource)
success = paint_core(left, top, right, bottom, resource, ROI_left, ROI_top, ROI_right, ROI_bottom, alpha, resampling_method)
nil = ReleaseFont(font)
resource = get_resource(id)
success = set_clip_rect_core(left, top, right, bottom)
success = set_movie_rect(left, top, right, bottom)
font = CreateFont(table{height, width, escapement, orientation, weight, italic, underline, strikeout, charset, outprecision, clipprecision, quality, pitch, name})
}

player = {
nil = play()
nil = pause()
nil = stop()
current_time = tell()
total_time = total()
success = seek(target)
bool = is_playing()
movie_loaded = bool (variable)
nil = reset()
success, HRESULT = reset_and_loadfile(file1, file2, stop)
success, HRESULT = load_subtitle(filename, reset)
success, HRESULT = load_audio_track(filename)
bool = is_fullscreen()
nil = toggle_fullscreen()
nil = toggle_3d()
true = set_swapeyes()
bool = get_swapeyes()
true or nil = set_volume(volume)
number = get_volume()
true or nil = show_mouse(show)
nil = popup_menu()
... = get_splayer_subtitle(filename)

--
succ = widi_start_scan()
... = widi_get_adapters()
info = widi_get_adapter_information(id, info)
succ = widi_connect(id, screen_mode)
succ = widi_set_screen_mode(screen_mode)
succ = widi_disconnect(id or all)
succ = enable_audio_track(id)
succ = enable_subtitle_track(id)
name,connected,name2,connected2,... = list_audio_track()
name,connected,name2,connected2,... = list_subtitle_track()
main = find_main_movie(path)
nil = restart_this_program()
path, volume_name, is_bluray, ... = enum_bd
path, ... = enum_drive()
{file, ...} = enum_folder(path)
succ = set_output_channel()
channel = get_output_channel()

succ = set_input_layout()
succ = set_output_mode()
succ = set_mask_mode()
succ = set_zoom_factor()
succ = set_subtitle_pos()
succ = set_subtitle_parallax()
succ = set_subtitle_latency_stretch()

layout = get_input_layout()
mode = get_output_mode()
mode = get_mask_mode
factor = get_zoom_factor()
x, y = get_subtitle_pos()
dp = get_subtitle_parallax()
latency, stretch = get_subtitle_latency_stretch()
left, top, right, bottom, description, ... = enum_monitors(horizontal_span, vertical_span)

-- TODO:
nil = show_color_adjust_dialog()
nil = show_hd3d_fullscreen_mode_dialog()
latency, ratio = show_latency_ratio_dialog(for_audio, latency, ratio)
button = message_box(content, caption, buttons)





-- move to renderer & lua

SwapEyes
Force2D
MoviePosX
MoviePosY

DisplaySubtitle
Aspect
AspectRatioMode
MovieResampling

-- dx9
dx9.lockframe()
dx9.unlockframe()
}


ui = {
handle = CreateMenu(root_handle)
success = AppendMenu(handle, string, flags, item_table)
success = AppendSubmenu(handle, string, flags, sub_handle)
nil = PopupMenu(handle, dx, dy)
nil = DestroyMenu(root_handle)
folder = OpenFolder()
url = OpenURL()
file = OpenFile(description1, extension1, description2, extension2, ...)
left, right = OpenDoubleFile()
int width
int height
x, y = get_mouse_pos()
}

core = {
success = ApplySetting(setting_name)
string_buffer, response_code = http_request(url)
json_context = cjson()
bool = FAILED(HRESULT)
int = GetTickCount()
success, reason = execute_luafile()
nil = track_back()
nil = Sleep(millisecond)
handle = CreateThread(function, ...)
nil = SuspendThread(handle)
nil = ResumeThread(handle)
int = WaitForSingleObject(handle, timeout or INFINITE)
path = GetConfigFile()
suc = WriteConfigFile(string)
lang = GetSystemDefaultLCID()
--nil = TerminateThread(handle, exit_code)		-- never use this thing, it cause stack corruption and dead lock
string loading_file
}


Thread = {
ThreadObject = Create(func, ...)
nil = Resume()
nil = Suspend()
int = Wait(timeout or INFINITE)
nil = Sleep(millisecond)
}

BaseFrame = {
frame = Create()
nil = render(view)					-- modification not recommended
width, height = GetSize()
nil = SetSize(width, height)
nil = SetWidth(width)
nil = SetHeight(height)
nil = SetAlpha(alpha)				-- not yet implemented
nil = RenderThis(view)				-- override this to draw your own frame!
success = AddChild(frame, pos)		-- modification not recommended
is = IsParentOf(frame, includeParentParent)
success = RemoveChild(frame)
nil = RemoveAllChilds()
nil = RemoveFromParent()
frame = GetParent()
frame = GetChild(n)					-- index start from 1
count = GetChildCount()
nil = BringToTop()
hit = HitTest(x, y)					-- override this if your need abnormal hit region
frame = GetFrameByPoint(x, y)		-- coordinates is screen coordinate. modification not recommended unless you need to block child frame from recieving mouse events
x, y = GetPoint(point)				-- coordinates is screen coordinate
left, top, right, bottom = GetRect()-- coordinates is screen coordinate
nil = CalculateAbsRect()			-- no need to call this, it is automatically called when layout changed.
is = IsMouseOver()
nil = BroadCastEvent(event, ...)	--broadcast a event to all childs, you can use this function, don't modify this function

-- events handlers, override to handle events
-- some of them require call to BaseFrame.[handler_name](self, ...)
block = OnLayoutChange()		-- return call
block = OnDropFile(x, y, ...)	--call
block = OnMouseDown(button, x, y)
block = OnMouseUp(button, x, y)
block = OnClick(button, x, y)
block = OnDoubleClick(button, x, y)
block = OnMouseOver()
block = OnMouseLeave()
block = OnMouseMove()
block = OnMouseWheel()
block = OnKeyDown()
block = OnKeyUp()
block = OnKillFocus()

-- don't use these function directly!, it's for event delivering
block = OnMouseEvent(event,x,y,...)
nil = OnEvent(event, ...)
nil = BroadcastLayoutEvent(event, ...) -- similar to BroadCastEvent, but through layout childs
block = OnMouseEvent(event,x,y,...)

-- don't use these function directly!, it's for relative layout internal management
success = AddLayoutChild(frame)
is = IsLayoutParentOf(frame)
nil = RemoveLayoutChild(frame)
{frames} = GetLayoutParents()
frame = GetLayoutChild(n)					-- index start from 1
count = GetLayoutChildCount()
success = SetPoint(point, frame, anchor, dx, dy)
}

-- helper functions
has_loop = frame_has_loop_reference(frame)			-- detect layout reference loop