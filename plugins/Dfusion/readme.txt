Dfusion command runs file <df dir>\dfusion\init.lua
Other than std lua commands supported:
	* Console.
		print(string)
		printerr(string)
		clear()
		gotoxy(x,y)
		color(int)	//TODO add id's, use numbers upto 16 for now
		reset_color()
		cursor(true/false)
		msleep(int)
		get_columns()
		get_rows()
		lineedit(string)	//text input through console
		history_add(string) // adds string to console history
	* Process.
		readDWord
		writeDWord
		readFloat
		readWord
		writeWord
		readByte
		writeByte
		read
		write
		read/writeSTLString
		copySTLString
		doReadClassName
		readClassName
		readCString
		isSuspended
		isIdentified
		getThreadIDs
		getMemRanges
		getBase
		getPath
		setPermisions
Functions searched:
	OnTick()
		If defined is called each DFHack tick.