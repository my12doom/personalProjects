-- base class
BaseFrame =
{

}

function debug(...)
	print("--DEBUG", ...)
end

function BaseFrame:Create(o)
	o = o or {}
	o.childs = {}
	setmetatable(o, self)
	self.__index = self

	return o
end

function BaseFrame.render(self, arg)
	self:RenderThis(self, arg)

	for i=1,#self.childs do
		local v = self.childs[i]
		if v and v.render then v:render(arg) end
	end
end

function BaseFrame.RenderThis(self, arg)
	debug("default rendering " .. tostring(self));
end

function BaseFrame.AddChild(self, frame)
	if frame == nil then return end
	if frame.parent ~= nil then
		debug("illegal AddChild(), frame already have a parent : adding " .. tostring(frame) .. " to ".. tostring(self))
		return
	end

	frame.parent = self;
	table.insert(self.childs, frame)
end

function BaseFrame.RemoveChild(self, frame)
	if frame == nil then return end
	if frame.parent ~= self then		-- illegal removal
		debug("illegal RemoveChild() " .. tostring(frame) .. " from ".. tostring(self))
		return
	end

	for i=1,#self.childs do
		if self.childs[i] == frame then
			table.remove(self.childs, i)
			break;
		end
	end

	frame.parent = nil
end

function BaseFrame.RemoveFromParent(self)
	if self.parent then
		self.parent:RemoveChild(self)
	else
		debug("illegal RemoveFromParent() : " .. tostring(self) .. " has no parent")
	end
end

function BaseFrame.GetParent(self)
	return self.parent
end

function BaseFrame.GetChild(self, n)
	return self.childs[n]
end

function BaseFrame.GetChildCount(self)
	return #self.childs
end

function BaseFrame.GetID(self)
	return self.id
end





-- frame creation and rendering implementation
root = BaseFrame:Create()
b = BaseFrame:Create()
c = BaseFrame:Create()


root.RenderThis = function(self, arg)
	print("--rendering Root")
end

b.RenderThis = function(self, arg)
	print("--rendering B")
end

c.RenderThis = function(self, arg)
	print("--rendering C")
end


d = BaseFrame:Create({RenderThis = function(self,arg)
	print("--rendering D")
end});



function list_frame(frame, n)
	n = n or 0
	local i
	prefix = ""

	for i=1,n do
		prefix = "  " .. prefix
	end
	print(prefix .. tostring(frame))

	for i=1,frame:GetChildCount() do
		list_frame(frame:GetChild(i), n+1)
	end
end

-- layout testing
root:AddChild(d)
root:AddChild(b)
root:AddChild(c)
root:AddChild(d)
list_frame(root)
root:render();

root:RemoveChild(c)
root:RemoveChild(c)
c:RemoveFromParent()
c:RemoveFromParent()
print("REMOVED c");
list_frame(root)
root:render();

d:AddChild(c)
print("ADDED c to D");
list_frame(root)
root:render();

b:RemoveFromParent();
c:AddChild(b);
list_frame(root)
root:render();

print(root, b, c, d)
print(b:GetParent():GetParent():GetParent())
