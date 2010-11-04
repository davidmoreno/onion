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
 * Input from the browser that will be sned to the remote terminal.
 */

// Normal shift status
shift=false
// alt gr status
altgr=false
/// Control status. this is a switch
cntrl=false


/// Parses key presses.
keypress = function(event){
	var keyCode=event.keyCode
	//showMsg('Key pressed '+keyCode)
	var keyValue=''
	if (keyCode==16){
		shift=true
	}
	else if (keyCode==17){
		cntrl=!cntrl
		if (cntrl)
			showMsg('Control ON')
		else
			showMsg('Control OFF')
	}
	else if (cntrl && keyCode){
		keyValue='\0'+(keyCode-64).toString(8)
		cntrl=false
		showMsg('Sent control '+String.fromCharCode(keyCode))
	}
	else if (shift && keyCode in keyCodesToValuesShift){
		keyValue=keyCodesToValuesShift[keyCode]
	}
	else if (keyCode in keyCodesToValues){
		//addText("&nbsp;")
		keyValue=keyCodesToValues[keyCode]
	}
	else if (keyCode>=65 && keyCode<=90 && (!event.shift)){
		keyCode+32
		keyValue=String.fromCharCode(keyCode+32)
	}
	else
		keyValue=String.fromCharCode(keyCode)
	
	/// If any text, do the petition.
	requestNewData(keyValue)
}

/// We also have to think about some keys that can be released later, like shift.
keyrelease = function(event){
	var keyCode=event.keyCode
	if (keyCode==16)
		shift=false
}

$(document).ready(function(){
	$(document).keydown(keypress)
	$(document).keyup(keyrelease)
})
