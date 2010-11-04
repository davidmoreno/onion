/** 

On this file it is defined all behavioural things about terminal. This is for linux terminal, but other terminals should use something similar.

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

/// Terminal data. First color attributes.

classesAttr={ 1: 'bright',
							2: 'dim',
							3: 'underline',
							5: 'blink',
							7: 'reverse',
							8: 'hidden'
}
classesBg={	40:'bg-black',
						41:'bg-red',
						42:'bg-green',
						43:'bg-yellow',
						44:'bg-blue',
						45:'bg-magenta',
						46:'bg-cyan',
						47:'bg-white'
}
classesColor={
						30:'black',
						31:'red',
						32:'green',
						33:'yellow',
						34:'blue',
						35:'magenta',
						36:'cyan',
						37:'white'
}

specialFuncs={
						'K':removeOneChar,
						'H':clearScreen,
						'J':gotoTopLeft
}

ignoreType1 = [ 'C' ]
ignoreType2 = [ 0 ]

/// Key codes to direct translate
keyCodesToValues={
										32:  ' ',
										190: '.',
										189: '-',
										191: '/',
										219: '[',
										220: '\\',
										221: ']',
										222: '"',
										187: '=',
										188: '<',
										190: '>',
										38: '\033[A',
										37: '\033[D',
										40: '\033[B',
										39: '\033[C'
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
	48: ')'
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
