set history save on

define nx_bt
  set $frame = $fp
  set $prev_frame = 0
  while $frame != 0 && $prev_frame != $frame
      set $prev_frame = $frame
      p/x ((unsigned long long *)$frame)[1]
      set $frame = ((unsigned long long *)$frame)[0]
  end
end

define wait_smo
	mon wait 0100000000010000
end
