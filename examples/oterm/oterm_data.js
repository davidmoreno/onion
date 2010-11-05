/*
	Onion HTTP terminal
	Copyright (C) 2010 David Moreno Montero

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	*/

/** 

On this file it is defined all behavioural things about terminal. This is for linux terminal, but other terminals should use something similar.

Quite important link:

http://www.databeast.com/datacomet/Documents/1.1._VT100_Command_Set.txt

*/
/**
 * Messages I dont know
 ?1049
 1;24r\033(B\033[
 4l\033[?7
 ?1034
 ?1
 H[2J[23d(B[0;7
 24;1H(B[
 [3d
 5d
 6d
 7d
 9d
 10d
 */

/**
                                  Terminal data. 
*/

/// First color attributes.
classesAttr={ 1: 'bright',
							2: 'dim',
							3: 'underline',
							5: 'blink',
							7: 'reverse',
							8: 'hidden',
							27: 'reverse-off'
}
classesBg={	40:'bg-black',
						41:'bg-red',
						42:'bg-green',
						43:'bg-yellow',
						44:'bg-blue',
						45:'bg-magenta',
						46:'bg-cyan',
						47:'bg-white',
						49:''
}
classesColor={
						30:'black',
						31:'red',
						32:'green',
						33:'yellow',
						34:'blue',
						35:'magenta',
						36:'cyan',
						37:'white',
						39:''
}

specialFuncs={
						'm':setColorStatus,
						'H':setPositionStatus,
						'r':setPositionStatus,
						'P':removeCharsStatus,
						'X':eraseCharacters,
						'K':removeToEOL,
						'J':clearScreen,
						'@':scrollLeft,
						'D':scrollLeft,
						'C':scrollRight, 
						'T':scrollDown,
						'S':scrollUp,
						'd':setVerticalPosition,
						'G':setHorizontalPosition,
						"'":setHorizontalPosition,
						'`':setHorizontalPosition,
						'l':lowModeStatus,
						'h':highModeStatus,
						'>':keyPadModeNumeric,
						'=':keyPadModeApplication,
						'?1049h':clearScreen
}


lowModeFuncs={
	'1':cursorKeyModeCursor,
	'4':overwriteMode,
	'7':autowrapOff,
	'12':remoteEcho,
	'25':hideCursor

}
highModeFuncs={
	'1':cursorKeyModeApplication,
	'4':insertMode,
	'7':autowrapOn,
	'12':localEcho,
	'25':showCursor
}

ignoreType1 = [ ]
ignoreType2 = [ ]


/**
                                                     INPUT DATA
 */

/// Key codes to direct translate
keyCodesToValues={
										13: '\n',
										32:  ' ',
										190: '.',
										189: '-',
										191: '/',
										219: '[',
										220: '\\',
										221: ']',
										222: '"',
										186: ':',
										187: '=',
										188: ',',
										190: '.',
										38: '\033[A', // key up, left, down, right
										37: '\033[D',
										40: '\033[B',
										39: '\033[C',
										46: '\033[3~'  // supr.

}

/// Im getting inside key mapping stuff. Should be configurable, or even better get it from config.
keyCodesToValuesShift={
	49: '!',
	50: '"',
	51: '·',
	52: '$',
	53: '%',
	54: '&',
	55: '&',
	57: '(',
	48: ')',
	188: '<', // not sure this one.. this is from the US mapping
	190: '>'
}

/// Im getting inside key mapping stuff. Should be configurable, or even better get it from config.
keyCodesToValuesAltgr={
	49: '|',
	50: '@',
	51: '#',
	52: '~',
	53: '½',
	54: '¬',
	55: '{',
	57: '[',
	48: ']',
	49: '}'
}

/// Im getting inside key mapping stuff. Should be configurable, or even better get it from config.
keyCodesToValuesControl={
	'A':'\001',
	'B':'\002',
	'C':'\003',
	'D':'\004',
	'E':'\005',
	'F':'\006',
	'G':'\007',
	'H':'\010',
	'I':'\011',
	'J':'\012',
	'K':'\013',
	'L':'\014',
	'M':'\015',
	'N':'\016',
	'O':'\017',
	'P':'\020',
	'Q':'\021',
	'R':'\022',
	'S':'\023',
	'T':'\024',
	'U':'\025',
	'V':'\026',
	'W':'\027',
	'X':'\030',
	'Y':'\031',
	'Z':'\032'
}
