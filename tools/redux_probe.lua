-- Runs inside PCSX-Redux. Saves raw screenshots at given vsync counts, then quits.
-- PROBE_OUT = output prefix, PROBE_FRAMES = comma separated vsync counts.
local out = os.getenv("PROBE_OUT") or "probe"
local wanted, last = {}, 0
for n in string.gmatch(os.getenv("PROBE_FRAMES") or "600", "%d+") do
	n = tonumber(n)
	wanted[n] = true
	if n > last then last = n end
end

local frames = 0
ProbeListener = PCSX.Events.createEventListener("GPU::Vsync", function()
	frames = frames + 1
	local press = tonumber(os.getenv("PROBE_SELECT_AT") or "0")
	if press > 0 then
		local pad = PCSX.SIO0.slots[1].pads[1]
		local select = PCSX.CONSTS.PAD.BUTTON.SELECT
		if frames == press then pad.setOverride(select) end
		if frames == press + 30 then pad.clearOverride(select) end
	end
	if wanted[frames] then
		local shot = PCSX.GPU.takeScreenShot()
		local path = out .. "-" .. frames .. "-" .. shot.width .. "x" .. shot.height .. ".raw"
		local f = Support.File.open(path, "TRUNCATE")
		f:writeMoveSlice(shot.data)
		f:close()
		print(string.format("PROBE frame=%d pc=%08x file=%s", frames, PCSX.getRegisters().pc, path))
	end
	local script = tonumber(os.getenv("PROBE_PLAY_AT") or "0")
	if script > 0 then
		local pad = PCSX.SIO0.slots[1].pads[1]
		local b = PCSX.CONSTS.PAD.BUTTON
		local t = frames - script
		if t == 0 then pad.setOverride(b.START) end
		if t == 10 then pad.clearOverride(b.START) end
		if t == 75 then pad.setOverride(b.CROSS); pad.setOverride(b.R1); pad.setOverride(b.RIGHT) end
		if t >= 75 and t % 12 == 0 then pad.setOverride(b.CIRCLE); pad.setOverride(b.TRIANGLE) end
		if t >= 75 and t % 12 == 6 then pad.clearOverride(b.CIRCLE); pad.clearOverride(b.TRIANGLE) end
	end
	if frames >= last then PCSX.quit(0) end
end)
